#include "SampleSimilarity.h"
#include "ToolBase.h"
#include <QFile>
#include <QTextStream>
#include <QFileInfo>
#include "Helper.h"

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
		setDescription("Calculates pairwise sample similarity metrics from VCF/BAM files.");
		setExtendedDescription(QStringList() << "In VCF mode, multi-allelic variants are not supported. Use VcfBreakMulti to split multi-allelic variants into several lines."
											 << "Multi-sample VCFs are not supported. Use VcfExtractSamples to split them to one VCF per sample.");
		addInfileList("in", "Input variant lists in VCF format (two or more).", false, true);
		//optional
		addOutfile("out", "Output file. If unset, writes to STDOUT.", true);
		addEnum("mode", "Input file format overwrite.", true, QStringList() << "vcf" << "bam", "vcf");
		addFlag("include_gonosomes", "Includes gonosomes into calculation (by default only variants on autosomes are considered).");
		addFlag("skip_multi", "Skip multi-allelic variants instead of throwing an error (VCF mode).");
		addInt("window", "Window to consider around indel positions to compensate for differing alignments (VCF mode).", true, 100);
		addInt("min_cov",  "Minimum coverage to consider a SNP for the analysis (BAM mode).",  true,  30);
		addInt("max_snps",  "The maximum number of high-coverage SNPs to analyze. 0 means unlimited (BAM mode).",  true,  500);
		addInfile("roi", "Target region used to speed up calculations e.g. for panel data (BAM mode).", true);
		addEnum("build", "Genome build used to generate the input (BAM mode).", true, QStringList() << "hg19" << "hg38", "hg19");


		//changelog
		changeLog(2018, 11, 26, "Add flag 'skip_multi' to ignore multi-allelic sites.");
		changeLog(2018,  7, 11, "Added build switch for hg38 support.");
		changeLog(2018,  6, 20, "Added IBS0 and IBS2 metrics and renamed tool to SampleSimilarity (was SampleCorrelation).");
		changeLog(2018,  1,  5, "Added multi-sample support and VCF input file support.");
		changeLog(2017,  7, 22, "Added 'roi' parameter.");
	}

	virtual void main()
	{
		//init
		QStringList in = getInfileList("in");
		QSharedPointer<QFile> outfile = Helper::openFileForWriting(getOutfile("out"), true);
		QTextStream out(outfile.data());
		QString mode = getEnum("mode");
		int window = getInt("window");
		int min_cov = getInt("min_cov");
		int max_snps = getInt("max_snps");
		QString roi = getInfile("roi");
		bool include_gonosomes = getFlag("include_gonosomes");
		QString build = getEnum("build");
		bool skip_multi = getFlag("skip_multi");

		//write header
		if (mode=="vcf")
		{
			out << "#file1\tfile2\toverlap_percent\tcorrelation\tibs2_percent\tcount1\tcount2\tcomments" << endl;
		}
		else if (mode=="bam")
		{
			out << "#file1\tfile2\tvariant_count\tcorrelation\tibs0_percent\tibs2_percent\tcomments" << endl;
		}
		else
		{
			THROW(ProgrammingException, "Invalid mode " + mode + "!");
		}


		//process
		for (int i=0; i<in.count(); ++i)
		{
			for (int j=i+1; j<in.count(); ++j)
			{
				SampleSimilarity sc;
				QStringList cols;
				cols << QFileInfo(in[i]).fileName();
				cols << QFileInfo(in[j]).fileName();
				if (mode=="vcf")
				{
					sc.calculateFromVcf(in[i], in[j], window, include_gonosomes, skip_multi);

					cols << QString::number(sc.olPerc(), 'f', 2);
					cols << QString::number(sc.sampleCorrelation(), 'f', 4);
					cols << QString::number(sc.ibs2Perc(), 'f', 2);
					cols << QString::number(sc.noVariants1());
					cols << QString::number(sc.noVariants2());
				}
				else if (mode=="bam")
				{
					sc.calculateFromBam(build, in[i], in[j], min_cov, max_snps, include_gonosomes, roi);

					cols << QString::number(sc.noVariants1());
					cols << QString::number(sc.sampleCorrelation(), 'f', 4);
					cols << QString::number(sc.ibs0Perc(), 'f', 2);
					cols << QString::number(sc.ibs2Perc(), 'f', 2);
				}
				else
				{
					THROW(ProgrammingException, "Invalid mode " + mode + "!");
				}
				cols << sc.messages().join(", ");
				out << cols.join("\t") << endl;
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
