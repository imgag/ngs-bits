#include "Exceptions.h"
#include "ToolBase.h"
#include "Helper.h"
#include <QFile>
#include <QTextStream>
#include <QFileInfo>
#include "Statistics.h"

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
		setDescription("Determines the gender of a sample from the BAM/CRAM file.");
		addInfileList("in", "Input BAM/CRAM file(s).", false, true);
		//optional
		addOutfile("out", "Output TSV file - one line per input BAM/CRAM file. If unset, writes to STDOUT.", true);
		QStringList methods;
		methods << "xy" << "hetx" << "sry";
		addEnum("method", "Method selection: Read distribution on X and Y chromosome (xy), fraction of heterozygous variants on X chromosome (hetx), or coverage of SRY gene (sry).", false, methods);
		addFloat("max_female", "Maximum Y/X ratio for female (method xy).", true, 0.06);
		addFloat("min_male", "Minimum Y/X ratio for male (method xy).", true, 0.09);
		addFloat("min_female", "Minimum heterozygous SNP fraction for female (method hetx).", true, 0.25);
		addFloat("max_male", "Maximum heterozygous SNP fraction for male (method hetx).", true, 0.05);
		addFloat("sry_cov", "Minimum average coverage of SRY gene for males (method sry).", true, 20.0);
		addEnum("build", "Genome build used to generate the input (methods hetx and sry).", true, QStringList() << "hg19" << "hg38", "hg19");
		addInfile("ref", "Reference genome for CRAM support (mandatory if CRAM is used).", true);

		//changelog
		changeLog(2020,  11, 27, "Added CRAM support.");
		changeLog(2018,  7, 13, "Change of output to TSV format for batch support.");
		changeLog(2018,  7, 11, "Added build switch for hg38 support.");
	}

	virtual void main()
	{
		//init
		QStringList in = getInfileList("in");
		QString method = getEnum("method");
		QSharedPointer<QFile> outfile = Helper::openFileForWriting(getOutfile("out"), true);
		QTextStream stream(outfile.data());

		//process
		bool print_header = true;
		foreach(QString bam, in)
		{
			//estimate
			GenderEstimate estimate;
			if (method=="xy")
			{
				estimate = Statistics::genderXY(bam, getFloat("max_female"), getFloat("min_male"), getInfile("ref"));
			}
			else if (method=="hetx")
			{
				estimate = Statistics::genderHetX(bam, getEnum("build"), getFloat("max_male"), getFloat("min_female"), getInfile("ref"));
			}
			else if (method=="sry")
			{
				estimate = Statistics::genderSRY(bam, getEnum("build"), getFloat("sry_cov"), getInfile("ref"));
			}

			//output header
			if (print_header)
			{
				stream << "#file\tgender";
				foreach (auto info, estimate.add_info)
				{
					stream << "\t" << info.key;
				}
				stream << endl;
				print_header = false;
			}

			//output estimate
			stream  << QFileInfo(bam).fileName() << "\t" << estimate.gender;
			foreach (auto info, estimate.add_info)
			{
				stream << "\t" << info.value;
			}
			stream << endl;
		}
	}
};

#include "main.moc"

int main(int argc, char *argv[])
{
	ConcreteTool tool(argc, argv);
	return tool.execute();
}
