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
		addInfile("in1", "Input variant list in TSV format.", false, true);
		addInfile("in2", "Input variant list in TSV format.", false, true);
		//optional
		addOutfile("out", "Output file. If unset, writes to STDOUT.", true);
		addFlag("bam", "Input files are BAM files instead of TSV files.");
		addInt("window", "Window to consider around indel positions to compensate for differing alignments (TSV input).", true, 100);
		addInt("min_cov",  "Minimum coverage to consider a SNP for the analysis (BAM input).",  true,  30);
		addInt("max_snps",  "The maximum number of high-coverage SNPs to analyze. 0 means unlimited (BAM input).",  true,  500);
		addInfile("roi", "Target region used to speed up calculations e.g. for panel data (BAM intput).", true);

		changeLog(2017,  7, 22, "Added 'roi' parameter.");
	}

	virtual void main()
	{
		//init
		QString in1 = getInfile("in1");
		QString in2 = getInfile("in2");

		QSharedPointer<QFile> outfile = Helper::openFileForWriting(getOutfile("out"), true);
		QTextStream out(outfile.data());

		//TSV mode
		SampleCorrelation sc;
		if (getFlag("bam")==false)
		{
			int window = getInt("window");
			sc.calculateFromVcf(in1,in2,window);

			//print results to command line
			out << "#overlap_percent\toverlap_correlation\tcount1\tcount2\tfile1\tfile2" << endl;
			out << QString::number(sc.olPerc(), 'f', 2) << "\t" << QString::number(sc.sampleCorrelation(), 'f', 3) << "\t" << QString::number(sc.noVariants1()) << "\t" << QString::number(sc.noVariants2()) << "\t" << QFileInfo(in1).fileName() << "\t" << QFileInfo(in2).fileName() << endl;
		}
		//BAM mode
		else
		{
			//parse parameters
			int min_cov = getInt("min_cov");
			int max_snps = getInt("max_snps");
			sc.calculateFromBam(in1, in2, min_cov, max_snps, getInfile("roi"));

			//output
			out << "Number of high-coverage SNPs: " << QString::number(sc.noVariants1()) << " of " << QString::number(sc.totalVariants()) << " (max_snps: " << QString::number(max_snps) << ")" << endl;
			out << "Correlation: " << QString::number(sc.sampleCorrelation(), 'f', 4) << endl;
		}

		//print messages
		foreach(QString message, sc.messages())
		{
			out << message << endl;
		}
	}

};

#include "main.moc"

int main(int argc, char *argv[])
{
	ConcreteTool tool(argc, argv);
	return tool.execute();
}
