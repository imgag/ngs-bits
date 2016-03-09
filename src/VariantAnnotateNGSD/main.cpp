#include "Exceptions.h"
#include "ToolBase.h"
#include "ChromosomalIndex.h"
#include "VariantList.h"
#include "BedFile.h"
#include "NGSD.h"
#include "GPD.h"
#include "Log.h"
#include "Settings.h"

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
		setDescription("Annotates a variant list with information from the NGSD.");
		addInfile("in", "Input variant list.", false, true);
		addOutfile("out", "Output variant list.", false, true);
		//optional
		addString("psname", "Processed sample name. If set, this name is used instead of the file name to find the sample in the DB.", true, "");
		addEnum("mode", "Determines annotation mode.", true, QStringList() << "germline" << "somatic", "germline");
		addFlag("test", "Uses the test database instead of on the production database.");
	}

	virtual void main()
	{
		//init
		QString psname = getString("psname");
		if (psname=="") psname = getInfile("in");

		//load
		VariantList variants;
		variants.load(getInfile("in"));

		//annotate
		bool test = getFlag("test");
		QString mode = getEnum("mode");
		if(mode=="germline")
		{
			try
			{
				if (!test) GPD().annotate(variants);
			}
			catch (DatabaseException& e)
			{
				Log::error("GPD database error: " + e.message());
			}
			NGSD(test).annotate(variants, psname);
		}
		else if(mode=="somatic")
		{
			try
			{
				if (!test) GPD().annotateSomatic(variants);
			}
			catch (DatabaseException& e)
			{
				Log::error("GPD database error: " + e.message());
			}
			NGSD(test).annotateSomatic(variants, psname);
		}

		//store
		variants.store(getOutfile("out"));
	}
};

#include "main.moc"

int main(int argc, char *argv[])
{
	ConcreteTool tool(argc, argv);
	return tool.execute();
}
