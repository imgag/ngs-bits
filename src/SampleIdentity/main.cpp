#include "ToolBase.h"
#include "Helper.h"
#include "VcfFile.h"
#include <QFileInfo>
#include "BasicStatistics.h"
#include "BamWorker.h"
#include <QThreadPool>

class ConcreteTool
		: public ToolBase
{
	Q_OBJECT

public:
	ConcreteTool(int& argc, char *argv[])
		: ToolBase(argc, argv)
	{
	}

	virtual void setup()
	{
		setDescription("Tries to identify datasets that are from the same patient based on BAM/CRAM files of WGS/WES/lrGS/RNA sequencing.");
		setExtendedDescription(QStringList() << "This tool works for HG38 only!" << "It calculates the identity of 75 SNPs that are usually well covered in lrGS, WGS, WES and RNA. It is much faster and memory-efficient than the SampleSimilarity tool, but does not work for panels. It should be used for checking for sample identitiy only. It cannot give information about relatedness of samples.");

		addInfileList("bams", "Input BAM/CRAM files. If only one file is given, it must be a text file with one BAM/CRAM path per line.", false, false);
		//optional
		addOutfile("out", "Output TSV file. If unset, writes to STDOUT.", true);
		addInt("min_depth",  "Minimum depth to use a SNP for the sample comparison.",  true,  15);
		addInt("min_snps",  "Minimum SNPs required to comare samples.",  true,  40);
		addInt("min_identity",  "Minimum identity percentage to show sample pairs in output.",  true,  95);
		addInt("threads", "Number of threads to calculate Allele  frequencies", true, 4);
		addFloat("min_correlation",  "Minimum correlation to show sample pairs in output.",  true,  0.9);
		addInfile("ref", "Reference genome for CRAM support (mandatory if CRAM is used).", true);
		addFlag("basename", "Use BAM/CRAM basename instead of full path in output.");
		addFlag("debug", "Add debug output to STDOUT. If used, make sure to provide a file for 'out'!");
		addFlag("time", "Add timing output to STDOUT. If used, make sure to provide a file for 'out'!");

		//changelog
		changeLog(2026,  2,  9, "Initial commit.");
	}

	virtual void main()
	{
		//init
		QStringList bams = getInfileList("bams");
		if (bams.count()==1)
		{
			bams = Helper::loadTextFile(bams[0], true, '#', true);
		}
		int min_depth = getInt("min_depth");
		int min_snps = getInt("min_snps");
		int min_identity = getInt("min_identity");
		int threads = getInt("threads");
		double min_correlation = getFloat("min_correlation");
		QSharedPointer<QFile> outfile = Helper::openFileForWriting(getOutfile("out"), true);
		QTextStream out_stream(outfile.data());
		bool basename = getFlag("basename");
		bool debug = getFlag("debug");
		bool time = getFlag("time");
		QTextStream debug_stream(stdout);

		if (threads < 1) THROW(ArgumentException, "Parameter 'threads' has to be greater than 0");

		//load SNPs
		VcfFile snps;
		snps.load(":/Resources/hg38_snps_identity.vcf");
		if (debug)
		{
			debug_stream << "Used SNPs (" << snps.count() << "):" <<  Qt::endl;
			for(int i=0; i<snps.count(); ++i)
			{
				debug_stream << "  " << snps[i].toString() <<  Qt::endl;
			}
		}

		//determine SNPs AFs from BAM/CRAMs
		if (time)
		{
			out_stream << "##" << Helper::toString(QDateTime::currentDateTime()) << " - loading SNPs from BAM/CRAMs..." << Qt::endl;
		}
		QList<QString> labels;
		labels.resize(bams.count());
		typedef QList<signed char> AfData; //AF rounded to int (0-100), or -1 for low coverage
		QList<AfData> af_data;
		af_data.resize(bams.count());

		QElapsedTimer timer;
		if (time) timer.start();
		int bams_done = 0;

		QList<bool> valid_bams;
		valid_bams.resize(bams.count());
		QThreadPool job_pool;
		QMutex out_stream_mtx;
		QMutex debug_stream_mtx;
		QMutex bams_done_mtx;

		job_pool.setMaxThreadCount(threads);

		for (int i =0; i < bams.count(); ++i){
			labels[i] = (basename ? QFileInfo(bams[i]).baseName() : bams[i]);
			af_data[i].resize(snps.count());
			job_pool.start(new BamWorker( {bams[i], af_data[i], valid_bams[i], snps, getInfile("ref"),
										  out_stream, out_stream_mtx, debug_stream, debug_stream_mtx,
										  timer, bams_done, bams_done_mtx, min_depth, debug, time } ) );
		}
		job_pool.waitForDone();

		//determine sample identity (and correlation if requested)
		if (time)
		{
			out_stream << "##" << Helper::toString(QDateTime::currentDateTime()) << " - calculating correlations..." << Qt::endl;
		}
		QVector<double> v1;
		QVector<double> v2;
		out_stream << "#file1\tfile2\tsnps_used\tidentity_percentage\tcorrelation";
		out_stream << Qt::endl;
		for (int i=0; i<af_data.count(); ++i)
		{
			if (!valid_bams[i]) continue;
			for (int j=i+1; j<af_data.count(); ++j)
			{
				if (!valid_bams[j]) continue;
				int snps_used = 0;
				int snps_identidy = 0;
				v1.clear();
				v2.clear();
				for(int k=0; k<af_data[i].count(); ++k)
				{
					signed char af1 = af_data[i][k];
					signed char af2 =  af_data[j][k];
					if (af1>-1 && af2>-1)
					{
						++snps_used;
						if (af1<10 && af2<10) ++snps_identidy; //both wt
						else if (af1>90 && af2>90) ++snps_identidy; //both hom
						else if (af1>=10 && af1<=90 && af2>=10 && af2<=90) ++snps_identidy; //both het
						else if (debug)
						{
							debug_stream << "Mismatch of " << snps[k].toString() << " af1=" << af1 << " af2=" << af2 << Qt::endl;
						}

						v1 << af1;
						v2 << af2;
					}
				}

				//check if we want to report the pair
				if (snps_used<min_snps) continue;
				double identity_perc = 100.0*snps_identidy/snps_used;
				if (identity_perc<min_identity) continue;
				double correlation = BasicStatistics::correlation(v1, v2);
				if (correlation<min_correlation) continue;

				//output
				out_stream << labels[i] << '\t' << labels[j] << '\t' << snps_used << '\t' << QString::number(identity_perc, 'f', 2) << '\t' <<  QString::number(correlation, 'f', 4) << Qt::endl;
			}
		}
		if (time)
		{
			out_stream << "##" << Helper::toString(QDateTime::currentDateTime()) << " - done" << Qt::endl;
		}
	}
};

#include "main.moc"

int main(int argc, char *argv[])
{
	ConcreteTool tool(argc, argv);
	return tool.execute();
}
