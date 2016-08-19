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
		addInfileList("in1", "Forward input gzipped FASTQ file(s).", false);
		addInfileList("in2", "Reverse input gzipped FASTQ file(s).", true);
		//optional
		addOutfile("out", "Output qcML file. If unset, writes to STDOUT.", true);
		addFlag("txt", "Writes TXT format instead of qcML.");

		changeLog(2016,  8, 19, "Added support for multiple input files.");
	}

	virtual void main()
	{
		//init
		StatisticsReads stats;
		FastqEntry entry;
		QStringList infiles;
		QStringList in1 = getInfileList("in1");
		QStringList in2 = getInfileList("in2");
		if (in2.count()!=0 && in1.count()!=in2.count())
		{
			THROW(CommandLineParsingException, "Input file lists 'in1' and 'in2' differ in counts!");
		}

		//process
		for (int i=0; i<in1.count(); ++i)
		{
			//forward
			FastqFileStream stream(in1[i]);
			while(!stream.atEnd())
			{
				stream.readEntry(entry);
				stats.update(entry, StatisticsReads::FORWARD);
			}
			infiles << in1[i];

			//reverse (optional)
			if (i<in2.count())
			{
				FastqFileStream stream2(in2[i]);
				while(!stream2.atEnd())
				{
					 stream2.readEntry(entry);
					 stats.update(entry, StatisticsReads::REVERSE);
				}

				//check read counts matches
				if (stream.index()!=stream2.index())
				{
					THROW(ArgumentException, "Differing number of reads in file '" + in1[i] + "' and '" + in2[i] + "'!");
				}

				infiles << in2[i];
			}
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

