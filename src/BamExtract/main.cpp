#include "ToolBase.h"
#include "BamWriter.h"

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
		setDescription("Extract reads from BAM/CRAM by read name.");
		addInfile("in", "Input BAM/CRAM file.", false);
		addInfile("ids", "Input text file containing read names (one per line).", false);
		addOutfile("out", "Output BAM/CRAM file with matching reads.", false);
		addOutfile("out2", "Output BAM/CRAM file with not matching reads.", true);
		addInfile("ref", "Reference genome for CRAM support (mandatory if CRAM is used).", true);

		changeLog(2023, 11, 30, "Initial implementation.");
	}

	virtual void main()
	{
		//init
		QTextStream out(stdout);
		QString ref = getInfile("ref");
		QString out2 = getOutfile("out2");

		//load read ids
		QSet<QByteArray> ids;
		QSharedPointer<QFile> file = Helper::openFileForReading(getInfile("ids"));
		while (!file->atEnd())
		{
			QByteArray line = file->readLine().trimmed();
			if (line.isEmpty() || line[0]=='#') continue;

			ids << line;
		}
		file->close();

		//open intput/output streams
		BamReader reader(getInfile("in"), ref);
		BamWriter writer(getOutfile("out"), ref);
		writer.writeHeader(reader);

		QSharedPointer<BamWriter> writer2;
		if (out2!="")
		{
			writer2 = QSharedPointer<BamWriter>(new BamWriter(out2, ref));
			writer2->writeHeader(reader);
		}

		//process alignments
		BamAlignment al;
		while (reader.getNextAlignment(al))
		{
			if (ids.contains(al.name()))
			{
				writer.writeAlignment(al);
			}
			else if (!writer2.isNull())
			{
				writer2->writeAlignment(al);
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
