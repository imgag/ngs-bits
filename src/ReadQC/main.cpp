#include "ToolBase.h"
#include "StatisticsReads.h"
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
		setDescription("Calculates QC metrics on unprocessed paired-end reads (same number of cycles/reads).");
		addInfile("in1", "Forward reads FASTQ file (gzipped or plain).", false, true);
		addInfile("in2", "Reverse reads FASTQ file (gzipped or plain).", false, true);
		//optional
		addOutfile("out", "Output qcML file. If unset, writes to STDOUT.", true);
		addFlag("txt", "Writes TXT format instead of qcML.");
	}

	virtual void main()
	{
		//init
		StatisticsReads stats;
		FastqEntry entry;

		//process forward read file
		QString forward_file = getInfile("in1");
		FastqFileStream stream(forward_file);
		while(!stream.atEnd())
		{
			stream.readEntry(entry);
			stats.update(entry, StatisticsReads::FORWARD);
		}
		int read_count_in1 = stream.index();

		//process reverse read file
		QString reverse_file = getInfile("in2");
		FastqFileStream stream2(reverse_file);
		while(!stream2.atEnd())
		{
			 stream2.readEntry(entry);
			 stats.update(entry, StatisticsReads::REVERSE);
		}

		//check read count matches
		if (read_count_in1!=stream.index())
		{
			THROW(ArgumentException, "Differing number of reads in file '" + forward_file + "' and '" + reverse_file + "'!");
		}

		//store output
		QCCollection metrics = stats.getResult();
		if (getFlag("txt"))
		{
			QStringList output;
			metrics.appendToStringList(output);
			Helper::storeTextFile(getOutfile("out"), output, true);
		}
		else
		{
			metrics.storeToQCML(getOutfile("out"), QStringList() << getInfile("in1") << getInfile("in2"), "");
		}
	}
};

#include "main.moc"

int main(int argc, char *argv[])
{
	ConcreteTool tool(argc, argv);
	return tool.execute();
}

