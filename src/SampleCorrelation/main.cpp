#include "SampleCorrelation.h"
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
		setDescription("Calculates the variant overlap and correlation of two variant lists.");
		addInfileList("in", "Input variant lists in GSvar format (two or more).", false, true);
		//optional
		addOutfile("out", "Output file. If unset, writes to STDOUT.", true);
		addEnum("mode", "Input file format overwrite.", true, QStringList() << "gsvar" << "vcf" << "bam", "gsvar");
		addInt("window", "Window to consider around indel positions to compensate for differing alignments (GSvar mode).", true, 100);
		addInt("min_cov",  "Minimum coverage to consider a SNP for the analysis (BAM mode).",  true,  30);
		addInt("max_snps",  "The maximum number of high-coverage SNPs to analyze. 0 means unlimited (BAM mode).",  true,  500);
		addInfile("roi", "Target region used to speed up calculations e.g. for panel data (BAM mode).", true);

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

		//write header
		if (mode=="gsvar" || mode=="vcf")
		{
			out << "#file1\tfile2\toverlap_percent\toverlap_correlation\tcount1\tcount2\tcomments" << endl;
		}
		else if (mode=="bam")
		{
			out << "#file1\tfile2\tvariant_count\toverlap_correlation\tcomments" << endl;
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
				SampleCorrelation sc;
				QStringList cols;
				cols << QFileInfo(in[i]).fileName();
				cols << QFileInfo(in[j]).fileName();
				if (mode=="gsvar" || mode=="vcf")
				{
					sc.calculateFromVcf(in[i], in[j], window);

					cols << QString::number(sc.olPerc(), 'f', 2);
					cols << QString::number(sc.sampleCorrelation(), 'f', 4);
					cols << QString::number(sc.noVariants1());
					cols << QString::number(sc.noVariants2());
				}
				else if (mode=="bam")
				{
					sc.calculateFromBam(in[i], in[j], min_cov, max_snps, roi);

					cols << QString::number(sc.noVariants1());
					cols << QString::number(sc.sampleCorrelation(), 'f', 4);
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
