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
		setDescription("Checks for and corrects errors/inconstistencies in the NGSD database.");
		//optional
		addFlag("fix", "Correct found errors/inconstistencies.");
		addFlag("test", "Uses the test database instead of on the production database.");
	}

	virtual void main()
	{
		//init
		NGSD db(getFlag("test"));
		QTextStream stream (stdout);
		db.maintain(&stream, getFlag("fix"));
	}
};

#include "main.moc"

int main(int argc, char *argv[])
{
	ConcreteTool tool(argc, argv);
	return tool.execute();
}
