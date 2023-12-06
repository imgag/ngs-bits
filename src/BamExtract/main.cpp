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
		QTextStream stdout_stream(stdout);
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
		stdout_stream << "Read IDs: " << ids.count() << endl;

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
		long long c_match = 0;
		long long c_other = 0;

		BamAlignment al;
		while (reader.getNextAlignment(al))
		{
			if (ids.contains(al.name()))
			{
				writer.writeAlignment(al);
				++c_match;
			}
			else if (!writer2.isNull())
			{
				writer2->writeAlignment(al);
				++c_other;
			}
		}

		stdout_stream << "Reads written to 'out': " << c_match<< endl;
		if (out2!="")
		{
			stdout_stream << "Reads written to 'out2': " << c_other << endl;
		}
	}
};

#include "main.moc"

int main(int argc, char *argv[])
{
	ConcreteTool tool(argc, argv);
	return tool.execute();
}
