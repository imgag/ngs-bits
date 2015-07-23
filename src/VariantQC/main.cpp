#include "ToolBase.h"
#include "Statistics.h"
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
		setDescription("Calculates QC metrics on variant lists.");
		addInfile("in", "Input VCF variant list. If a specific column ", false, false);
		//optional
		addOutfile("out", "Output qcML file. If unset, writes to STDOUT.", true);
		addFlag("txt", "Writes TXT format instead of qcML.");
	}

	virtual void main()
	{
		//load variant list
		VariantList vl;
		QString filename = getInfile("in");
		vl.load(filename);

		//calculate metrics
		QCCollection metrics = Statistics::variantList(vl);

		//store output
		if (getFlag("txt"))
		{
			QStringList output;
			metrics.appendToStringList(output);
			Helper::storeTextFile(getOutfile("out"), output, true);
		}
		else
		{
			metrics.storeToQCML(getOutfile("out"), QStringList() << getInfile("in"), "");
		}
	}
};

#include "main.moc"

int main(int argc, char *argv[])
{
	ConcreteTool tool(argc, argv);
	return tool.execute();
}

