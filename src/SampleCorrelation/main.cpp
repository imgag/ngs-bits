#include "Exceptions.h"
#include "ToolBase.h"
#include "ChromosomalIndex.h"
#include "VariantList.h"
#include "BasicStatistics.h"
#include "Helper.h"
#include <QFile>
#include <QTextStream>
#include <QFileInfo>
#include "NGSHelper.h"

#include "api/BamReader.h"
using namespace BamTools;

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
		setDescription("Calculates the variant overlap and correlation of two variant lists.");
		addInfile("in1", "Input variant list in TSV format.", false, true);
		addInfile("in2", "Input variant list in TSV format.", false, true);
		//optional
		addOutfile("out", "Output file. If unset, writes to STDOUT.", true);
		addFlag("bam", "Input files are BAM files instead of TSV files.");
		addInt("window", "Window to consider around indel positions to compensate for differing alignments (TSV input).", true, 100);
		addInt("min_cov",  "Minimum coverage to consider a SNP for the analysis. (BAM input)",  true,  30);
		addInt("max_snps",  "The maximum number of high-coverage SNPs to analyze. 0 means unlimited. (BAM input)",  true,  500);
	}

	virtual void main()
	{
		//init
		QString in1 = getInfile("in1");
		QString in2 = getInfile("in2");

		QStringList messages;
		QSharedPointer<QFile> outfile = Helper::openFileForWriting(getOutfile("out"), true);
		QTextStream out(outfile.data());

		//TSV mode
		if (getFlag("bam")==false)
		{
			//load input files
			VariantList file1;
			file1.load(in1);
			VariantList file2;
			file2.load(in2);

			//get genotype column indices
			int col_geno1 = file1.annotationIndexByName("genotype", true, true);
			int col_geno2 = file2.annotationIndexByName("genotype", true, true);

			//calculate overlap / correlation
			int c_ol = 0;
			QVector<double> geno1;
			QVector<double> geno2;
			ChromosomalIndex<VariantList> file2_idx(file2);
			for (int i=0; i<file1.count(); ++i)
			{
				Variant& v1 = file1[i];
				int start = v1.start();
                int end = v1.end();

				//indel => fuzzy position search
				if (!v1.isSNV())
				{
					start -= getInt("window");
					end += getInt("window");
				}

				QVector<int> matches = file2_idx.matchingIndices(v1.chr(), start, end);
				foreach(int index, matches)
				{
					Variant& v2 = file2[index];
					if (v1.ref()==v2.ref() && v1.obs()==v2.obs())
					{
						++c_ol;
						geno1.append(genoToDouble(v1.annotations()[col_geno1]));
						geno2.append(genoToDouble(v2.annotations()[col_geno2]));
						break;
					}
				}
			}
			double ol_perc = 100.0 * c_ol / std::min(file1.count(), file2.count());
			double correlation = BasicStatistics::correlation(geno1, geno2);

			//calulate percentage with same genotype if correlation is not calculatable
			if (!BasicStatistics::isValidFloat(correlation))
			{
				double equal = 0.0;
				for (int i=0; i<geno1.count(); ++i)
				{
					equal += (geno1[i]==geno2[i]);
				}
				correlation = equal / geno1.count();
				messages.append("Note: Could not calulate the genotype correlation, calculated the fraction of matching genotypes instead.");
			}

			//print results to command line
			out << "#overlap_percent\toverlap_correlation\tcount1\tcount2\tfile1\tfile2" << endl;
			out << QString::number(ol_perc, 'f', 2) << "\t" << QString::number(correlation, 'f', 3) << "\t" << QString::number(file1.count()) << "\t" << QString::number(file2.count()) << "\t" << QFileInfo(in1).fileName() << "\t" << QFileInfo(in2).fileName() << endl;
		}
		//BAM mode
		else
		{
			//parse parameters
			int min_cov = getInt("min_cov");
			int max_snps = getInt("max_snps");
			VariantList snps = NGSHelper::getSNPs();

			//open BAM readers
			BamReader r1;
			NGSHelper::openBAM(r1, in1);
			BamReader r2;
			NGSHelper::openBAM(r2, in2);

			//calcualate frequencies
			QVector<double> freq1;
			freq1.reserve(max_snps);
			QVector<double> freq2;
			freq2.reserve(max_snps);
			for(int i=0; i<snps.count(); ++i)
			{
				//out << "SNP " << snp.chr << " " << QString::number(snp.pos) << endl;
				Pileup p1 = NGSHelper::getPileup(r1, snps[i].chr(), snps[i].start());
				//out << " D1: " <<p1.depth() << endl;
				if (p1.depth(false)<min_cov) continue;

				Pileup p2 = NGSHelper::getPileup(r2, snps[i].chr(), snps[i].start());
				//out << " D2: " <<p2.depth() << endl;
				if (p2.depth(false)<min_cov) continue;

				QChar ref = snps[i].ref()[0];
				QChar obs = snps[i].obs()[0];
				double p1_freq = p1.frequency(ref, obs);
				double p2_freq = p2.frequency(ref, obs);

				//skip non-informative snps
				if (!BasicStatistics::isValidFloat(p1_freq) || !BasicStatistics::isValidFloat(p2_freq)) continue;

				freq1.append(p1_freq);
				freq2.append(p2_freq);
				if (freq1.count()==max_snps) break;
			}

			//output
			out << "Number of high-coverage SNPs: " << QString::number(freq1.count()) << " of " << QString::number(snps.count()) << " (max_snps: " << QString::number(max_snps) << ")" << endl;
			double correlation = BasicStatistics::correlation(freq1, freq2);
			out << "Correlation: " << QString::number(correlation, 'f', 4) << endl;
		}

		//print messages
		foreach(QString message, messages)
		{
			out << message << endl;
		}
	}

	static double genoToDouble(const QString& geno)
	{
		if (geno=="hom") return 1.0;
		if (geno=="het") return 0.5;
		if (geno=="lofreq") return 0.1;

		THROW(ArgumentException, "Invalid genotype '" + geno + "' in input file.");
	}
};

#include "main.moc"

int main(int argc, char *argv[])
{
	ConcreteTool tool(argc, argv);
	return tool.execute();
}
