#include "Exceptions.h"
#include "ToolBase.h"
#include "NGSD.h"
#include "Helper.h"
#include "Exceptions.h"
#include "Settings.h"
#include <QSqlQuery>
#include <QSqlRecord>
#include <QDir>

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
		setDescription("Lists genes from NGSD.");
		addOutfile("out", "Output TSV file. If unset, writes to STDOUT.", true);
		//optional
		addFlag("test", "Uses the test database instead of on the production database.");
	}

	virtual void main()
	{
		//init
		NGSD db(getFlag("test"));
		SqlQuery query = db.getQuery();

		//write header
		QSharedPointer<QFile> output = Helper::openFileForWriting(getOutfile("out"), true);
		output->write("#symbol\tHGNC id\ttype\tname\n");

		//write content
		query.exec("SELECT symbol, hgnc_id, type, name FROM gene ORDER BY symbol ASC");
		while (query.next())
		{
			output->write(query.value(0).toByteArray() + "\tHGNC:" + query.value(1).toByteArray() + "\t" + query.value(2).toByteArray() + "\t" + query.value(3).toByteArray() + "\n");
		}
	}
};

#include "main.moc"

int main(int argc, char *argv[])
{
	ConcreteTool tool(argc, argv);
	return tool.execute();
}
