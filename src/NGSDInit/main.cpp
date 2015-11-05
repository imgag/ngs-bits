#include "ToolBase.h"
#include "NGSD.h"
#include "Exceptions.h"

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
		setDescription("Sets up the NDSD database (creates tables and adds minimal data).");
		//optional
		addInfile("add", "Additional SQL script to execute after database initialization.", true);
		addString("force", "Database password needed to re-initialize the production database.", true, "");
		addFlag("test", "Uses the test database instead of on the production database.");
	}

	virtual void main()
	{
		//init
		NGSD db(getFlag("test"));
		db.init(getString("force"));

		//add data
		QString add = getInfile("add");
		if (add!="")
		{
			db.executeQueriesFromFile(add);
		}

		//output
		QTextStream out(stdout);
		out << "Database initialization succesfully." << endl;
		out << "You are now able to login as user 'admin' and password 'admin' via the web fronted." << endl;
		out << "**Change password on first login!**" << endl;
	}
};

#include "main.moc"

int main(int argc, char *argv[])
{
	ConcreteTool tool(argc, argv);
	return tool.execute();
}
