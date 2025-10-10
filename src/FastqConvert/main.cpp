#include "ToolBase.h"
#include "FastqFileStream.h"

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
		setDescription("Converts the quality scores from Illumina 1.5 offset to Sanger/Illumina 1.8 offset.");
		addInfile("in", "Input gzipped FASTQ file.", false);
		addOutfile("out", "Output gzipped FASTQ file.", false);
		//optional
		addInt("compression_level", "Output FASTQ compression level from 1 (fastest) to 9 (best compression).", true, 1);

		changeLog(2020, 7, 15, "Added 'compression_level' parameter.");
	}

	virtual void main()
	{
		//process
		int compression_level = getInt("compression_level");
		FastqOutfileStream outstream(getOutfile("out"), compression_level);
		FastqFileStream stream( getInfile("in"), false);
		FastqEntry entry;
		while (!stream.atEnd())
		{
			stream.readEntry(entry);
            for (int i=0; i<entry.qualities.size(); ++i)
			{
				entry.qualities[i] = (char)(entry.qualities[i]) - 31;
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
