#include "ToolBase.h"
#include "Helper.h"
#include "Exceptions.h"
#include "VcfFile.h"
#include <QFile>
#include <QTextStream>
#include <QList>

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
		setDescription("Converts a VCF file to a tab-separated text file.");
        setExtendedDescription(QStringList() << "Multi-allelic variants are supported. All alternative sequences are stored as a comma-seperated list."
                                             << "Multi-sample VCFs are supported. For every combination of FORMAT and SAMPLE a seperate column is generated and named in the following way: <SAMPLEID>_<FORMATID>_<format>.");
		addInfile("in", "Input variant list in VCF format.", false, true);
		addOutfile("out", "Output variant list in TSV format.", false, true);

        changeLog(2020,  8, 07, "Multi-allelic and Multi-sample VCFs are supported.");
	}

	virtual void main()
	{
		//load
		VcfFile vl;
		vl.load(getInfile("in"));

		//store
		vl.storeAsTsv(getOutfile("out"));
	}
};

#include "main.moc"

int main(int argc, char *argv[])
{
	ConcreteTool tool(argc, argv);
	return tool.execute();
}

