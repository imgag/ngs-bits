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
		addOutfile("out", "Output file. If unset, writes to STDOUT.", true);
		addFlag("fix", "Correct found errors/inconstistencies.");
		addFlag("test", "Uses the test database instead of on the production database.");
	}

	virtual void main()
	{
		//init
		NGSD db(getFlag("test"));
		bool fix  = getFlag("fix");
		QSharedPointer<QFile> out = Helper::openFileForWriting(getOutfile("out"), true);
		QTextStream stream(out.data());
		db.maintain(&stream, fix);
	}
};

#include "main.moc"

int main(int argc, char *argv[])
{
	ConcreteTool tool(argc, argv);
	return tool.execute();
}
