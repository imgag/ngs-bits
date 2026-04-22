#include "ToolBase.h"
#include "Helper.h"
#include "FastqFileStream.h"
#include <QFile>
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
		setDescription("Determines the FastQ quality encoding format.");
		addInfile("in", "Input FASTQ file (gzipped or plain).", false);
		addOutfile("out", "Output text file. If unset, writes to STDOUT.", true);
		addInt("reads", "The number of reads to parse.", true, 10000);
	}

	virtual void main()
	{
		//init
		int reads = getInt("reads");

		// determine score min and max
		int min = 127;
		int max = 32;
		FastqFileStream stream(getInfile("in"), false);
        QStringList output;
		FastqEntry entry;
		while (!stream.atEnd() && stream.index()<reads)
		{
			stream.readEntry(entry);

			foreach(char c, entry.qualities)
			{

				int value = c;
                if (value<33 || value>104)
                {
                    THROW(FileParseException, QString::number(stream.index()) + ". Read contains illegal quality value: " + QString((char)c) + " ("+ QString::number(value) + ")");
                }
				min = std::min(min, value);
				max = std::max(max, value);
			}
		}

		// determine format
        output.append("Minimum ASCII character: " + QString((char)min) + " ("+ QString::number(min) + ")");
		output.append("Maximum ASCII character: " + QString((char)max) + " ("+ QString::number(max) + ")");
		if (min>=33 && max<=75)
		{
			output.append("Format: Sanger/Illumina1.8");
		}
        else if (min>=64 && max<=104)
		{
			output.append("Format: Illumina1.5");
		}
		else
		{
			output.append("Format: Unknown");
		}
		Helper::storeTextFile(Helper::openFileForWriting(getOutfile("out"), true), output);
	}
};

#include "main.moc"

int main(int argc, char *argv[])
{
	ConcreteTool tool(argc, argv);
	return tool.execute();
}
