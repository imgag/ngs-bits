#include "ToolBase.h"
#include "NGSD.h"
#include <QTextStream>

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
		setDescription("Precalculates variant counts for fast annotation.");
		//optional
		addInt("log_interval", "Log every n entries", true, 10000);
		addFlag("test", "Uses the test database instead of on the production database.");
	}

	virtual void main()
	{
		//init
		NGSD db(getFlag("test"));
		QTextStream stream (stdout);
		db.precalculateGenotypeCounts(&stream, getInt("log_interval"));
	}
};

#include "main.moc"

int main(int argc, char *argv[])
{
	ConcreteTool tool(argc, argv);
	return tool.execute();
}
