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
		addFlag("rm", "If set, the columns given in 'cols' are removed instead of extracted.");
	}

	virtual void main()
	{
		QString in = getInfile("in");
		QString out = getOutfile("out");
		if(in!="" && in==out)
		{
			THROW(ArgumentException, "Input and output files must be different when streaming!");
		}
		TSVFileStream instream(in);
		QSharedPointer<QFile> outstream = Helper::openFileForWriting(out, true);

		//check columns
		QVector<int> cols = instream.checkColumns(getString("cols").toUtf8().split(','), getFlag("numeric"));

		//remove instead of extract > invert column selection
		if (getFlag("rm"))
		{
			QSet<int> old = cols.toList().toSet();
			cols.clear();
			for (int c=0; c<instream.columns(); ++c)
			{
				if (!old.contains(c)) cols << c;
			}
		}

		//write comments
		foreach (QByteArray comment, instream.comments())
		{
			outstream->write(comment);
			outstream->write("\n");
		}

		//write header
		outstream->write("#");
		for(int i=0; i<cols.count(); ++i)
		{
			outstream->write(instream.header()[cols[i]]);
			outstream->write(i==cols.count()-1 ? "\n" : "\t");
		}

		//write content
		while(!instream.atEnd())
		{
			QList<QByteArray> parts = instream.readLine();
			if (parts.count()==0) continue;
			for(int i=0; i<cols.count(); ++i)
			{
				outstream->write(parts[cols[i]]);
				outstream->write(i==cols.count()-1 ? "\n" : "\t");
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

