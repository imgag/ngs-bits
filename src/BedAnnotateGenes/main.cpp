#include "BedFile.h"
#include "ToolBase.h"
#include "NGSHelper.h"
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
		setDescription("Annotates the regions in a BED file with the name from a database BED file.");
		addInfile("in", "Input BED file.", false, true);
		addOutfile("out", "Output BED file.", false, true);
		//optional
		addInfile("db", "Database BED file containing names in the 4th column. If unset 'ccds' from the 'settings.ini' file is used.", true, true);
		addInt("extend", "The number of bases to extend the database regions at start/end.", true, 0);
	}

	virtual void main()
	{
		//load DB file
		QString db_file = getInfile("db");
		if (db_file=="") db_file = Settings::string("ccds");

		BedFile db;
		db.load(db_file);

		//extend
		int extend = getInt("extend");
		if (extend>0)
		{
			db.extend(extend);
		}

		//create DB index
		ChromosomalIndex<BedFile> db_idx(db);

		BedFile file;
		file.load(getInfile("in"));

		for(int i=0; i<file.count(); ++i)
		{
			BedLine& line = file[i];
			QStringList genes = NGSHelper::genesForRegion(db_idx, line.chr(), line.start(), line.end());
			if (line.annotations().empty()) line.annotations().append("");
			line.annotations()[0] = genes.join(", ");
		}

		file.store(getOutfile("out"));
	}
};

#include "main.moc"

int main(int argc, char *argv[])
{
	ConcreteTool tool(argc, argv);
	return tool.execute();
}
