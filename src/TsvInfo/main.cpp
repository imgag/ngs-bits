#include "ToolBase.h"
#include "TSVFileStream.h"
#include "Helper.h"
#include "BasicStatistics.h"
#include <QFileInfo>
#include <QBitArray>

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
		setDescription("Prints general information about a TSV file.");
		//optional
		addInfile("in", "Input TSV file. If unset, reads from STDIN.", true);
		addOutfile("out", "Output file. If unset, writes to STDOUT.", true);
	}

	virtual void main()
	{
		QString in = getInfile("in");
		TSVFileStream instream(in);

		//process
		int rows = 0;
		QBitArray numeric(instream.columns(), true);
		while(!instream.atEnd())
		{
			QList<QByteArray> parts = instream.readLine();
			if (parts.count()==0) continue;
			++rows;

			for (int i=0; i<numeric.count(); ++i)
			{
				if (!numeric[i]) continue;
				numeric[i] = BasicStatistics::isValidFloat(parts[i]);
			}
		}

		//header
		QSharedPointer<QFile> out = Helper::openFileForWriting(getOutfile("out"), true);
		out->write("File   : " + QFileInfo(in).fileName().toLatin1() + "\n");
		out->write("Columns: " + QByteArray::number(instream.columns()) + "\n");
		out->write("Rows   : " + QByteArray::number(rows) + "\n");
		out->write("\n");

		//columns
		out->write("Column details:\n");
		for(int i=0; i<instream.columns(); ++i)
		{
			QByteArray num_info;
			if (numeric[i]) num_info = " (N)";
			out->write(QByteArray::number(i).rightJustified(2, ' ') + ": " + instream.header().at(i) + num_info + "\n");
		}
    }
};

#include "main.moc"

int main(int argc, char *argv[])
{
	ConcreteTool tool(argc, argv);
	return tool.execute();
}

