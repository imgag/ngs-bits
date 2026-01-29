#include "ToolBase.h"
#include "Helper.h"
#include "VcfFile.h"
#include "BamReader.h"
#include <QFileInfo>
#include "BasicStatistics.h"

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
		setDescription("Calculates pairwise sample similarity metrics from VCF/BAM/CRAM files.");
		setExtendedDescription(QStringList() << "This tool works for HG38 only!" << "It calculates the identity of 75 SNPs that are usually well covered in WGS, WES and RNA.");

		addInfileList("bams", "Input BAM/CRAM files. If only one file is given, it must be a text file with one BAM/CRAM path per line.", false, false);
		//optional
		addOutfile("out", "Output TSV file. If unset, writes to STDOUT.", true);
		addInt("min_depth",  "Minimum depth to use a SNP for the sample comparison.",  true,  15);
		addInt("min_snps",  "Minimum SNPs required to comare samples.",  true,  20);
		addInfile("ref", "Reference genome for CRAM support (mandatory if CRAM is used).", true);
		addFlag("basename", "Use BAM/CRAM basename instead of full path in output.");
		addFlag("add_correlation", "Report SNP correlation in addition to identity percentage.");
		addFlag("debug", "Add debug output to STDOUT. If used, make sure to provide a file for 'out'!");

		//changelog
		changeLog(2026, 01, 30, "Initial commit.");
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
		QSharedPointer<QFile> outfile = Helper::openFileForWriting(getOutfile("out"), true);
		QTextStream out_stream(outfile.data());
		bool basename = getFlag("basename");
		bool add_correlation = getFlag("add_correlation");
		bool debug = getFlag("debug");
		QTextStream debug_stream(stdout);

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
		QList<QString> labels;
		labels.reserve(bams.count());
		typedef QList<signed char> AfData; //AF rounded to int (0-100), or -1 for low coverage
		QList<AfData> af_data;
		af_data.reserve(bams.count());
		foreach(QString bam, bams)
		{
			//check BAM exists
			if (!QFile::exists(bam))
			{
				out_stream << "##skipped missing file: " << bam << "\n";
				continue;
			}

			//get AFs
			QElapsedTimer timer;
			if (debug) timer.start();

			BamReader reader(bam, getInfile("ref"));
			reader.skipQualities();
			reader.skipTags();

			AfData afs;
			afs.resize(snps.count());
			for(int i=0; i<snps.count(); ++i)
			{
				const Chromosome& chr = snps[i].chr();
				int pos = snps[i].start();
				char ref = snps[i].ref()[0];
				int ref_c = 0;
				char alt = snps[i].alt()[0][0];
				int alt_c = 0;

				reader.setRegion(chr, pos, pos);

				//iterate through all alignments and create counts
				BamAlignment al;
				while (reader.getNextAlignment(al))
				{
					if (al.isSecondaryAlignment() || al.isSupplementaryAlignment() || al.isDuplicate() || al.isUnmapped()) continue;

					QPair<char, int> base = al.extractBaseByCIGAR(pos);
					if (base.first==ref) ++ref_c;
					if (base.first==alt) ++alt_c;
				}

				//low depth => -1
				if (ref_c+alt_c<min_depth)
				{
					afs << -1;
					continue;
				}

				afs[i] = std::round(100.0*alt_c/(ref_c+alt_c));
			}

			if (debug)
			{
				debug_stream << "Determining SNPs for " << bam << " took " << Helper::elapsedTime(timer.elapsed()) << Qt::endl;
			}
			af_data << afs;
			labels << (basename ? QFileInfo(bam).baseName() : bam);
		}

		//determine sample identity (and correlation if requested)
		QVector<double> v1;
		QVector<double> v2;
		out_stream << "#file1\tfile2\tsnps_used\tidentity_percentage";
		if (add_correlation) out_stream << "\tcorrelation";
		out_stream << Qt::endl;
		for (int i=0; i<af_data.count(); ++i)
		{
			for (int j=i+1; j<af_data.count(); ++j)
			{
				int snps_used = 0;
				int snps_identidy = 0;
				if (add_correlation)
				{
					v1.clear();
					v2.clear();
				}
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
						if (add_correlation)
						{
							v1 << af1;
							v2 << af2;
						}
					}
				}

				if (snps_used<min_snps) continue;

				out_stream << labels[i] << '\t' << labels[j] << '\t' << snps_used << '\t' << QString::number(100.0*snps_identidy/snps_used, 'f', 2);
				if (add_correlation) out_stream << '\t' <<  QString::number(BasicStatistics::correlation(v1, v2), 'f', 4);
				out_stream << Qt::endl;
			}
		}
	}
};

#include "main.moc"

int main(int argc, char *argv[])
{
	ConcreteTool tool(argc, argv);
	return tool.execute();
}
