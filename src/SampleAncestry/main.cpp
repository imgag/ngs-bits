#include "ToolBase.h"
#include "VariantList.h"
#include "Helper.h"
#include "NGSHelper.h"
#include "Statistics.h"

#include <QTextStream>
#include <QFileInfo>

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
		setDescription("Estimates the ancestry of a sample based on variants.");

		addInfileList("in", "Input variant list(s) in VCF format.", false);
		//optional
		addOutfile("out", "Output TSV file. If unset, writes to STDOUT.", true);
		addInt("min_snps", "Minimum number of informative SNPs for population determination. If less SNPs are found, 'NOT_ENOUGH_SNPS' is returned.", true, 1000);
		addFloat("pop_dist", "Minimum relative distance between first/second population score. If below this score 'ADMIXED/UNKNOWN' is called.", true, 0.15);
		addEnum("build", "Genome build used to generate the input.", true, QStringList() << "hg19" << "hg38", "hg19");

		//changelog
        changeLog(2020,  8, 07, "VCF files only as input format for variant list.");
		changeLog(2018, 12, 10, "Fixed bug in handling of 'pop_dist' parameter.");
		changeLog(2018,  7, 11, "Added build switch for hg38 support.");
		changeLog(2018,  7, 03, "First version.");
	}

	virtual void main()
	{
		//init
		QStringList in = getInfileList("in");
		QSharedPointer<QFile> outfile = Helper::openFileForWriting(getOutfile("out"), true);
		QTextStream out(outfile.data());
		int min_snps = getInt("min_snps");
		double pop_dist = getFloat("pop_dist");
		QString build = getEnum("build");

		//process
		out << "#sample\tsnps\tAFR\tEUR\tSAS\tEAS\tpopulation" << endl; //TODO Add probabilty for each population > MARC
		foreach(QString filename, in)
		{
			AncestryEstimates ancestry = Statistics::ancestry(build, filename, min_snps, pop_dist);
			out << QFileInfo(filename).fileName()
				<< "\t" << ancestry.snps
				<< "\t" << QString::number(ancestry.afr, 'f', 4)
				<< "\t" << QString::number(ancestry.eur, 'f', 4)
				<< "\t" << QString::number(ancestry.sas, 'f', 4)
				<< "\t" << QString::number(ancestry.eas, 'f', 4)
				<< "\t" << ancestry.population << endl;
		}
	}
};

#include "main.moc"

int main(int argc, char *argv[])
{
    ConcreteTool tool(argc, argv);
    return tool.execute();
}

