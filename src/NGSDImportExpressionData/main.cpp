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
		QStringList mode = QStringList() << "genes" << "transcripts" << "exon";
		addEnum("mode", "Determines which kind of expression data should be imported.", true, mode, "genes");
		addFlag("force", "Import data even if already imported and overwrite data in the NGSD.");
		addFlag("test", "Uses the test database instead of on the production database.");
		addFlag("debug", "Enable debug output.");

		//changelog
		changeLog(2022, 5, 3, "Initial version.");
		changeLog(2022, 6, 17, "Added transcript support.");
	}

	virtual void main()
	{
		NGSD db(getFlag("test"));
		QString mode = getEnum("mode");
		if(mode == "genes")
		{
			db.importGeneExpressionData(getInfile("expression"), getString("ps"), getFlag("force"), getFlag("debug"));
		}
		else if(mode == "transcripts")
		{
			db.importTranscriptExpressionData(getInfile("expression"), getString("ps"), getFlag("force"), getFlag("debug"));
		}
		else if(mode == "exon")
		{
			db.importExonExpressionData(getInfile("expression"), getString("ps"), getFlag("force"), getFlag("debug"));
		}
		else
		{
			THROW(ArgumentException, "Invalid mode '" + mode + "given!")
		}

	}
};

#include "main.moc"

int main(int argc, char *argv[])
{
	ConcreteTool tool(argc, argv);
	return tool.execute();
}
