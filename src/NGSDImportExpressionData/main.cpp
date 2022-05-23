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
		setDescription("Imports expression data into the NGSD.");
		addInfile("expression", "TSV file containing expression values (TPM).", false, true);
		addString("ps", "Processed sample name of the expression data.", false);

		//optional
		addFlag("force", "Import data even if already imported and overwrite data in the NGSD.");
		addFlag("test", "Uses the test database instead of on the production database.");
		addFlag("debug", "Enable debug output.");

		//changelog
		changeLog(2022, 5, 3, "Initial version.");
	}

	virtual void main()
	{
		NGSD db(getFlag("test"));
		db.importExpressionData(getInfile("expression"), getString("ps"), getFlag("force"), getFlag("debug"));
	}
};

#include "main.moc"

int main(int argc, char *argv[])
{
	ConcreteTool tool(argc, argv);
	return tool.execute();
}
