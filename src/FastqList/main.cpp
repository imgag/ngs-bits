#include "ToolBase.h"
#include "Helper.h"
#include "FastqFileStream.h"
#include <QFile>

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
		setDescription("List read IDs and base count.");
		addInfile("in", "Input FASTQ file (gzipped or plain).", false);
		//optional
		addOutfile("out", "Output TSV file. If unset, writes to STDOUT.", true);
	}

	virtual void main()
	{
		QSharedPointer<QFile> file = Helper::openFileForWriting(getOutfile("out"), true);
		file->write("#id\tbases\n");

		//parse input and write output
		FastqFileStream stream(getInfile("in"));
		FastqEntry entry;
		while (!stream.atEnd())
		{
			stream.readEntry(entry);

			//remove @ and the comment part (after first space)
			QByteArray id = entry.header.trimmed().mid(1);
			int comment_start = id.indexOf(' ');
			if (comment_start!=-1) id = id.left(comment_start);

			file->write(id + "\t" + QByteArray::number(entry.bases.size()) + "\n");
		}
	}
};

#include "main.moc"

int main(int argc, char *argv[])
{
	ConcreteTool tool(argc, argv);
	return tool.execute();
}
