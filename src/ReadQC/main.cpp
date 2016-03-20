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
		addInfile("in2", "Reverse reads FASTQ file (gzipped or plain).", true, true);
		//optional
		addOutfile("out", "Output qcML file. If unset, writes to STDOUT.", true);
		addFlag("txt", "Writes TXT format instead of qcML.");
	}

	virtual void main()
	{
		//init
		StatisticsReads stats;
		FastqEntry entry;
		QStringList infiles;

		//process forward read file
		QString in1 = getInfile("in1");
		FastqFileStream stream(in1);
		while(!stream.atEnd())
		{
			stream.readEntry(entry);
			stats.update(entry, StatisticsReads::FORWARD);
		}
		infiles << in1;

		//process reverse read file
		QString in2 = getInfile("in2");
		if (in2!="")
		{
			FastqFileStream stream2(in2);
			while(!stream2.atEnd())
			{
				 stream2.readEntry(entry);
				 stats.update(entry, StatisticsReads::REVERSE);
			}

			//check read counts matches
			if (stream.index()!=stream2.index())
			{
				THROW(ArgumentException, "Differing number of reads in file '" + in1 + "' and '" + in2 + "'!");
			}

			infiles << in2;
		}

		//store output
		QCCollection metrics = stats.getResult();
		if (getFlag("txt"))
		{
			QStringList output;
			metrics.appendToStringList(output);
			Helper::storeTextFile(Helper::openFileForWriting(getOutfile("out"), true), output);
		}
		else
		{
			metrics.storeToQCML(getOutfile("out"), infiles, "");
		}
	}
};

#include "main.moc"

int main(int argc, char *argv[])
{
	ConcreteTool tool(argc, argv);
	return tool.execute();
}

