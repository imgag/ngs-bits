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
		setDescription("Extracts/reorders columns of a TSV file.");
		addString("cols", "Comma-separated list of column names to extract.", false);
		//optional
		addInfile("in", "Input TSV file. If unset, reads from STDIN.", true);
		addOutfile("out", "Output file. If unset, writes to STDOUT.", true);
		addFlag("numeric", "If set, column names are interpreted as 1-based column numbers.");
	}

	virtual void main()
	{
		TSVFileStream in(getInfile("in"));
		QSharedPointer<QFile> out = Helper::openFileForWriting(getOutfile("out"), true);

		//check columns
		QVector<int> cols = in.checkColumns(getString("cols"), getFlag("numeric"));
		const int col_count = cols.count();

		//write comments
		foreach (QByteArray comment, in.comments())
		{
			out->write(comment);
			out->write("\n");
		}

		//write header
		out->write("#");
		for(int i=0; i<col_count; ++i)
		{
			out->write(in.header()[cols[i]]);
			out->write(i==col_count-1 ? "\n" : "\t");
		}

		//write content
		while(!in.atEnd())
		{
			QList<QByteArray> parts = in.readLine();
			for(int i=0; i<col_count; ++i)
			{
				out->write(parts[cols[i]]);
				out->write(i==col_count-1 ? "\n" : "\t");
			}
		}
    }
};

#include "main.moc"

int main(int argc, char *argv[])
{
	ConcreteTool tool(argc, argv);
	return tool.execute();
}

