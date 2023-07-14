#include "ToolBase.h"
#include "Helper.h"
#include "FastqFileStream.h"
#include <QSet>
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
		setDescription("Extracts reads from a FASTQ file according to an ID list. Trims the reads if lengths are given.");
		addInfile("in", "Input FASTQ file (gzipped or plain).", false);
		addInfile("ids", "Input TSV file containing IDs (without the '@') in the first column and optional length in the second column.", false);
		addOutfile("out", "Output FASTQ file.", false);
		//optional
		addFlag("v", "Invert match: keep non-matching reads.");
		addInt("compression_level", "Output FASTQ compression level from 1 (fastest) to 9 (best compression).", true, Z_BEST_SPEED);
		addFlag("long_read", "Support long reads (> 1kb).");

		changeLog(2023,  4,  18, "Added support for long reads.");
		changeLog(2020, 7, 15, "Added 'compression_level' parameter.");
	}

	virtual void main()
	{
		//init
		bool v = getFlag("v");
		bool long_read = getFlag("long_read");

		//load ids and lengths
		QHash<QByteArray, int> ids;
		QSharedPointer<QFile> file = Helper::openFileForReading(getInfile("ids"));
		while (!file->atEnd())
		{
			QByteArray line = file->readLine().trimmed();
			if (line.isEmpty() || line[0]=='#') continue;
			QList<QByteArray> parts = line.split('\t');
			int length = -1;
			if (parts.count()>1)
			{
				length = Helper::toInt(parts[1], "length value");
			}
			ids.insert(parts[0], length);
		}

		//open output stream
		int compression_level = getInt("compression_level");
		FastqOutfileStream outfile(getOutfile("out"), compression_level);

		//parse input and write output
		FastqFileStream stream(getInfile("in"), true, long_read);
		FastqEntry entry;
		while (!stream.atEnd())
		{
			stream.readEntry(entry);

			QByteArray id = entry.header.trimmed();
			id = id.mid(1);
			int comment_start = id.indexOf(' ');
			if (comment_start!=-1) id = id.left(comment_start);
			int length = ids.value(id, -2);
			if (length==-2) //id not in list
			{
				if (!v) continue;

				outfile.write(entry);
			}
			else if (length==-1) //id is in list, but no length given
			{
				if (v) continue;

				outfile.write(entry);
			}
			else if (length>=1) //id is in list and length given
			{
				if (v) continue;

				entry.bases.resize(length);
				entry.qualities.resize(length);
				outfile.write(entry);
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
