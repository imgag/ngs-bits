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
		addInt("start", "Number of bases to trim from start of the read.", true, 0);
		addInt("end", "Number of bases to trim from the end of the read.", true, 0);
	}

	virtual void main()
	{
		//init
		int start = getInt("start");
		int end = getInt("end");

		//process
		FastqOutfileStream outstream(getOutfile("out"), false);
		FastqFileStream stream( getInfile("in"), false);
		FastqEntry entry;
		while (!stream.atEnd())
		{
			stream.readEntry(entry);

			int len = entry.bases.count();
			if (len>start+end)
			{
				entry.bases = entry.bases.mid(start, len-start-end);
				entry.qualities = entry.qualities.mid(start, len-start-end);
				outstream.write(entry);
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
