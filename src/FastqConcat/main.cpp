#include "ToolBase.h"
#include "Helper.h"
#include "FastqFileStream.h"
#include <QSet>
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
		setDescription("Concatinates a list of FASTQ files in one output FASTQ file.");
		addInfileList("in", "Input (gzipped) FASTQ files.", false);
		addOutfile("out", "Output gzipped FASTQ file.", false);

		//optional
		addInt("compression_level", "gzip compression level from 1 (fastest) to 9 (best compression).", true, 1);

		//changelog
		changeLog(2019, 4, 8, "Initial version of this tool");

	}

	virtual void main()
	{
		//init
		QStringList in_files = getInfileList("in");

		int compression_level = getInt("compression_level");
		if (compression_level<1 || compression_level>9) THROW(CommandLineParsingException, "Invalid compression level " + QString::number(compression_level) +"!");
		FastqOutfileStream output_stream(getOutfile("out"), compression_level);

		FastqEntry entry;
		foreach(QString in_file, in_files)
		{
			// get next input file:
			FastqFileStream input_stream(in_file, false);

			// write input file in output
			while (!input_stream.atEnd())
			{
				input_stream.readEntry(entry);
				output_stream.write(entry);
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
