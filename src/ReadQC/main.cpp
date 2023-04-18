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
		setDescription("Calculates QC metrics on unprocessed NGS reads.");
		addInfileList("in1", "Forward input gzipped FASTQ file(s).", false);
		addInfileList("in2", "Reverse input gzipped FASTQ file(s) for paired-end mode (same number of cycles/reads as 'in1').", true);
		//optional
		addOutfile("out", "Output qcML file. If unset, writes to STDOUT.", true);
		addFlag("txt", "Writes TXT format instead of qcML.");
		addOutfile("out1", "If set, writes merged forward FASTQs to this file (gzipped).", true);
		addOutfile("out2", "If set, writes merged reverse FASTQs to this file (gzipped)", true);
		addInt("compression_level", "Output FASTQ compression level from 1 (fastest) to 9 (best compression).", true, Z_BEST_SPEED);
		addFlag("long_read", "Support long reads (> 1kb).");

		changeLog(2023,  4,  18, "Added support for LongRead");
		changeLog(2021,  2,  3, "Added option to write out merged input FASTQs (out1/out2).");
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

		int compression_level = getInt("compression_level");
		FastqOutfileStream* out1_stream = nullptr;
		QString out1 = getOutfile("out1");
		bool write1 = out1!="";
		if (write1) out1_stream = new FastqOutfileStream(out1, compression_level);
		FastqOutfileStream* out2_stream = nullptr;
		QString out2 = getOutfile("out2");
		bool write2 = out2!="";
		if (write2) out2_stream = new FastqOutfileStream(out2, compression_level);
		bool long_read = getFlag("long_read");

		//process
		for (int i=0; i<in1.count(); ++i)
		{
			//forward
			FastqFileStream stream(in1[i], true, long_read);
			while(!stream.atEnd())
			{
				stream.readEntry(entry);
				stats.update(entry, StatisticsReads::FORWARD);

				if (write1)
				{
					out1_stream->write(entry);
				}
			}
			infiles << in1[i];

			//reverse (optional)
			if (i<in2.count())
			{
				FastqFileStream stream2(in2[i], true, long_read);
				while(!stream2.atEnd())
				{
					 stream2.readEntry(entry);
					 stats.update(entry, StatisticsReads::REVERSE);

					 if (write2)
					 {
						 out2_stream->write(entry);
					 }
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

		//close output st reas
		if (write1) out1_stream->close();
		if (write2) out2_stream->close();
	}
};

#include "main.moc"

int main(int argc, char *argv[])
{
	ConcreteTool tool(argc, argv);
	return tool.execute();
}

