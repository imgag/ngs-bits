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
		setDescription("Converts the quality scores from Illumina 1.5 offset to Sanger/Illumina 1.8 offset.");
		addInfile("in", "Input gzipped FASTQ file.", false);
		addOutfile("out", "Output gzipped FASTQ file.", false);
	}

	virtual void main()
	{
		//process
		FastqOutfileStream outstream(getOutfile("out"));
		FastqFileStream stream( getInfile("in"), false);
		FastqEntry entry;
		while (!stream.atEnd())
		{
			stream.readEntry(entry);
			for (int i=0; i<entry.qualities.count(); ++i)
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
