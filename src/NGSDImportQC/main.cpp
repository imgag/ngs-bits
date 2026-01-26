#include "ToolBase.h"
#include "NGSD.h"

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
		setDescription("Imports QC terms into the NGSD.");
		addInfile("obo", "HPO ontology file from 'https://raw.githubusercontent.com/imgag/ngs-bits/master/src/cppNGS/Resources/qcML.obo'.", false);

		//optional
		addFlag("test", "Uses the test database instead of on the production database.");
		addFlag("debug", "Enable debug output.");
	}

	virtual void main()
	{
		NGSD db(getFlag("test"));
		db.updateQC(getInfile("obo"), getFlag("debug"));
	}
};

#include "main.moc"

int main(int argc, char *argv[])
{
	ConcreteTool tool(argc, argv);
	return tool.execute();
}
