#include "ToolBase.h"
#include "FastqFileStream.h"
#include "Helper.h"
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
		setDescription("Converts a FASTQ file to FASTA format.");
		addInfile("in", "Input gzipped FASTQ file.", false);
		//optional
		addOutfile("out", "Output FASTA file. If unset, writes to STDOUT.", true);
	}

	virtual void main()
	{
		//open output stream
		QSharedPointer<QFile> outfile = Helper::openFileForWriting(getOutfile("out"), true);
		QTextStream outstream(outfile.data());

		//process
		FastqFileStream stream(getInfile("in"), false);
		FastqEntry entry;
		while (!stream.atEnd())
		{
			stream.readEntry(entry);
			outstream << ">" << entry.header.mid(1) << "\n";
			outstream << entry.bases << "\n";
		}
	}
};



#include "main.moc"

int main(int argc, char *argv[])
{
	ConcreteTool tool(argc, argv);
	return tool.execute();
}
