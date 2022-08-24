#include <QFile>
#include <QTextStream>
#include <QList>
#include <QDebug>
#include <QTime>
#include "ToolBase.h"
#include "VariantList.h"
#include "Exceptions.h"
#include "zlib.h"
#include "Helper.h"
#include "VcfToBedpe.h"
#include "BedpeFile.h"

//TODO output BEDPE is not 0-based > fix. Adapt BedpeFile::load to convert the coordinates while loading so that the class is 1-based > adapt affectedRegion method

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
		setDescription("Converts a VCF file containing structural variants to BEDPE format.");
		setExtendedDescription({"Input can be MANTA oder DELLY VCF files."});
		addInfile("in", "Input structural variant list in VCF format.", false, true);
		addOutfile("out", "Output structural variant list in BEDPE format.", false, true);
		addFlag("no_sort","Do not sort results");
	}

	virtual void main()
	{
		VcfToBedpe in_file(getInfile("in").toUtf8());
		in_file.convert(getOutfile("out").toUtf8());

		if(!getFlag("no_sort"))
		{
			BedpeFile file;
			file.load(getOutfile("out"));
			file.sort();
			file.toTSV(getOutfile("out"));
		}
	}

private:


};

#include "main.moc"

int main(int argc, char *argv[])
{
	ConcreteTool tool(argc, argv);
	return tool.execute();
}
