#include "ToolBase.h"
#include "NGSD.h"
#include "Exceptions.h"
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
		setDescription("Imports QC terms into the NGSD.");
		addInfile("obo", "HPO ontology file from 'https://github.com/imgag/megSAP/raw/master/data/dbs/Ontologies/qc-cv.obo'.", false);

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
