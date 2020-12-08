#include "ToolBase.h"
#include "FastqFileStream.h"
#include "Exceptions.h"

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
		setDescription("Trims start/end bases from all reads in a FASTQ file.");
		addInfile("in", "Input gzipped FASTQ file.", false);
		addOutfile("out", "Output gzipped FASTQ file.", false);
		//optional
		addInt("start", "Trim this number of bases from the start of the read.", true, 0);
		addInt("end", "Trim this number of bases from the end of the read.", true, 0);
		addInt("len", "Restrict read length to this value (after trimming from start/end).", true, 0);
		addInt("compression_level", "Output FASTQ compression level from 1 (fastest) to 9 (best compression).", true, Z_BEST_SPEED);

		changeLog(2020, 7, 15, "Added 'compression_level' parameter.");
		changeLog(2016, 8, 26, "Added 'len' parameter.");
	}

	virtual void main()
	{
		//init
		const int start = getInt("start");
		const int end = getInt("end");
		const int len = getInt("len");

		//process
		int compression_level = getInt("compression_level");
		FastqOutfileStream outstream(getOutfile("out"), compression_level);
		FastqFileStream stream( getInfile("in"), false);
		FastqEntry entry;
		while (!stream.atEnd())
		{
			stream.readEntry(entry);

			if (start>0 || end>0)
			{
				const int len2 = entry.bases.count();
				if (len2<=start+end) continue;
				entry.bases = entry.bases.mid(start, len2-start-end);
				entry.qualities = entry.qualities.mid(start, len2-start-end);
			}

			if (len>0 && entry.bases.count()>len)
			{
				entry.bases.resize(len);
				entry.qualities.resize(len);
			}

			outstream.write(entry);
		}
	}
};

#include "main.moc"

int main(int argc, char *argv[])
{
	ConcreteTool tool(argc, argv);
	return tool.execute();
}
