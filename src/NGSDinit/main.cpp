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
		setDescription("Sets up the NDSD database.");
		//optional
		addString("force", "Database password needed to re-initialize the production database.", true, "");
		addFlag("test", "Uses the test database instead of on the production database.");
	}

	virtual void main()
	{
		//init
		NGSD db(getFlag("test"));
		db.init(getString("force"));

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
