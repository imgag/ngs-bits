#include "ToolBase.h"
#include "FastqFileStream.h"
#include "Helper.h"

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
		setDescription("Downsamples paired-end FASTQ files.");
		addInfile("in1", "Forward input gzipped FASTQ file(s).", false);
		addInfile("in2", "Reverse input gzipped FASTQ file(s).", false);
		addFloat("percentage", "Percentage of reads to keep.", false);
		addOutfile("out1", "Forward output gzipped FASTQ file.", false);
		addOutfile("out2", "Reverse output gzipped FASTQ file.", false);
		//optional
		addFlag("test", "Test mode: fix random number generator seed and write kept read names to STDOUT.");
		addInt("compression_level", "Output FASTQ compression level from 1 (fastest) to 9 (best compression).", true, 1);

		changeLog(2020, 7, 15, "Initial version of this tool.");
	}

	virtual void main()
	{
		//init
		QString in1 = getInfile("in1");
		QString in2 = getInfile("in2");
		QString out1 = getOutfile("out1");
		QString out2 = getOutfile("out2");
		double percentage = getFloat("percentage");
		if (percentage<=0 || percentage>=100) THROW(CommandLineParsingException, "Invalid percentage " + QString::number(percentage) +"!");
		bool test = getFlag("test");
		int compression_level = getInt("compression_level");

		//open streams
		QTextStream out(stdout);
		FastqFileStream is1(in1, false);
		FastqFileStream is2(in2, false);
		FastqOutfileStream os1(out1, compression_level);
		FastqOutfileStream os2(out2, compression_level);

		//init random number generator
		srand(test ? 1 : QTime::currentTime().msec());

		//process
		int c_read_pairs = 0;
		int c_passed = 0;
		FastqEntry read1;
		FastqEntry read2;
		while (!is1.atEnd() && !is2.atEnd())
		{
			is1.readEntry(read1);
			is2.readEntry(read2);
			++c_read_pairs;

			if (Helper::randomNumber(0, 100)<percentage)
			{
				os1.write(read1);
				os2.write(read2);
				++c_passed;
                if (test) out << "KEPT PAIR: " << read1.header << Qt::endl;
			}
		}

		//check that forward and reverse read file are both at the end
		if (!is1.atEnd())
		{
			THROW(FileParseException, "File " + in1 + " has more entries than " + in2 + "!");
		}
		if (!is2.atEnd())
		{
			THROW(FileParseException, "File " + in2 + " has more entries than " + in1 + "!");
		}

		//write debug output
        out << "PE reads read   : " << c_read_pairs << Qt::endl;
        out << "PE reads written: " << c_passed << Qt::endl;
	}
};

#include "main.moc"

int main(int argc, char *argv[])
{
	ConcreteTool tool(argc, argv);
	return tool.execute();
}
