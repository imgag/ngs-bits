#include "ToolBase.h"
#include "Exceptions.h"
#include "Helper.h"
#include "BedFile.h"
#include "ChromosomalIndex.h"
#include "VcfFile.h"
#include "Auxilary.h"
#include "ChunkProcessor.h"
#include "OutputWorker.h"
#include <QFile>
#include <QSharedPointer>
#include <QThreadPool>

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
		setDescription("Annotates the INFO column of a VCF with data from a BED file.");
		QByteArray mapping_string;
		foreach (KeyValuePair replacement, VcfFile::INFO_URL_MAPPING)
		{
			mapping_string += replacement.key + " -> " + replacement.value + "; ";
		}
		setExtendedDescription(QStringList()	<< "Characters which are not allowed in the INFO column based on the VCF 4.2 definition are URL encoded."
												<< "The following characters are replaced:" << mapping_string);
		addInfile("bed", "BED file used as source of annotations (name column).", false, true);
		addString("name", "Annotation name in INFO column of output VCF file.", false);
		//optional
		addInfile("in", "Input VCF file. If unset, reads from STDIN.", true, true);
		addOutfile("out", "Output VCF list. If unset, writes to STDOUT.", true, true);
		addString("sep", "Separator used if there are several matches for one variant.", true, ":");
		addInt("threads", "The number of threads used to read, process and write files.", true, 1);
		addInt("block_size", "Number of lines processed in one chunk.", true, 5000);
		addInt("prefetch", "Maximum number of chunks that may be pre-fetched into memory.", true, 64);
		addInt("debug", "Enables debug output at the given interval in milliseconds (disabled by default, cannot be combined with writing to STDOUT).", true, -1);

		changeLog(2021,  9, 18, "Prefetch only part of input file (to save memory).");
		changeLog(2021,  8, 24, "Added multithread support.");
		changeLog(2021,  6, 15, "Added 'sep' parameter.");
		changeLog(2019, 12,  6, "Added URL encoding for INFO values.");
		changeLog(2017,  3, 14, "Initial implementation.");
	}

	virtual void main()
	{
		//init
		QString in = getInfile("in");
		QString out = getOutfile("out");
		QByteArray bed = getInfile("bed").toLatin1();
		QByteArray name = getString("name").toLatin1().trimmed();
		QByteArray sep = getString("sep").toLatin1().trimmed();
		int block_size = getInt("block_size");
		int threads = getInt("threads");
		int prefetch = getInt("prefetch");
		int debug = getInt("debug");

		// check parameter:
		if (block_size < 1) THROW(ArgumentException, "Parameter 'block_size' has to be greater than zero!");
		if (threads < 1) THROW(ArgumentException, "Parameter 'threads' has to be greater than zero!");
		if (prefetch < threads) THROW(ArgumentException, "Parameter 'prefetch' has to be at least number of used threads!");
		if (out.isEmpty() && (debug > 0)) THROW(ArgumentException, "Parameter 'progress' cannot be combined with writing to STDOUT!");

		//load BED file
		BedFile bed_data;
		bed_data.load(bed);
		if (!bed_data.isSorted()) bed_data.sort();
		ChromosomalIndex<BedFile> bed_index(bed_data);

		//check BED file
		for(int i=0; i<bed_data.count(); ++i)
		{
			BedLine& line = bed_data[i];
			if (line.annotations().count()==0)
			{
				THROW(FileParseException, "BED line '" + line.toString(true) + "' has no name column: " + line.toString(true));
			}
			if (line.annotations()[0].contains(sep))
			{
				THROW(FileParseException, "BED line '" + line.toString(true) + "' name column contains separator: " + line.annotations()[0]);
			}
		}

		//open input/output streams
		if(!in.isEmpty() && in==out)
		{
			THROW(ArgumentException, "Input and output files must be different when streaming!");
		}
		QSharedPointer<QFile> in_p = Helper::openFileForReading(in, true);

		// create job pool
		QList<AnalysisJob> job_pool;
		while(job_pool.count() < prefetch)
		{
			job_pool << AnalysisJob();
		}

		// create thread pool
		QThreadPool analysis_pool;
		analysis_pool.setMaxThreadCount(threads + 1); // +1 for output writer

		// start writer thread
		OutputWorker* output_worker = new OutputWorker(job_pool, out);
		analysis_pool.start(output_worker);

		//QByteArrayList data;
		int current_chunk = 0;

		//debug
		QTime timer;
		QTextStream outstream(stdout);
		if (debug>0) timer.start();

		try
		{
			// parse VCF file:
			while(!in_p->atEnd())
			{
				int to_be_processed = 0;
				int to_be_written = 0;
				int vcf_line_idx = 0;
				int done = 0;
				for (int j=0; j<job_pool.count(); ++j)
				{
					AnalysisJob& job = job_pool[j];
					switch(job.status)
					{
						case DONE:
							++done;

							// create new job with the next chunk of lines
							job.clear();
							job.chunk_id = current_chunk;
							job.status = TO_BE_PROCESSED;
							while(vcf_line_idx < block_size && !in_p->atEnd())
							{
								job.current_chunk.append(QByteArray(in_p->readLine()));
								vcf_line_idx++;
							}
							vcf_line_idx = 0;
							analysis_pool.start(new ChunkProcessor(job, name, bed_data, bed_index, bed, sep));
							++current_chunk;
							break;

						case TO_BE_WRITTEN:
							++to_be_written;
							//sleep
							QThread::msleep(100);
							break;

						case TO_BE_PROCESSED:
							++to_be_processed;
							//sleep
							QThread::msleep(100);
							break;

						case ERROR:
							if (job.error_message.startsWith("FileParseException\t"))
							{
								THROW(FileParseException, job.error_message.split("\t").at(1));
							}
							else if (job.error_message.startsWith("FileAccessException\t"))
							{
								THROW(FileAccessException, job.error_message.split("\t").at(1));
							}
							else
							{
								THROW(Exception, job.error_message);
							}
							break;
						default:
							break;
					}

					//break if file is read
					if(in_p->atEnd()) break;

				}

				//debug output
				if (debug>0 && timer.elapsed()>debug)
				{
					outstream << Helper::dateTime() << " debug - to_be_processed: " << to_be_processed << " to_be_written: " << to_be_written << " done: " << done << endl;
					timer.restart();
				}
			}

			// close file
			in_p->close();


			//wait for all jobs to finish
			if (debug>0) outstream << Helper::dateTime() << " input data read completely - waiting for analysis to finish" << endl;
			int done = 0;
			int to_be_written, to_be_processed;
			while(done < job_pool.count())
			{
				done = 0;
				to_be_written = 0;
				to_be_processed = 0;
				for (int j=0; j<job_pool.count(); ++j)
				{
					AnalysisJob& job = job_pool[j];
					switch(job.status)
					{
						case DONE:
							++done;
							break;

						case TO_BE_WRITTEN:
							to_be_written++;
							//sleep
							QThread::msleep(100);
							break;

						case TO_BE_PROCESSED:
							++to_be_processed;
							//sleep
							QThread::msleep(100);
							break;


						case ERROR:
							if (job.error_message.startsWith("FileParseException\t"))
							{
								THROW(FileParseException, job.error_message.split("\t").at(1));
							}
							else if (job.error_message.startsWith("FileAccessException\t"))
							{
								THROW(FileAccessException, job.error_message.split("\t").at(1));
							}
							else
							{
								THROW(Exception, job.error_message);
							}
							break;

						default:
							break;
					}
				}

				//sleep
				QThread::msleep(500);

				//debug output
				if (debug>0 && timer.elapsed()>debug)
				{
					outstream << Helper::dateTime() << " debug - to_be_analyzed: " << to_be_processed << " to_be_written: " << to_be_written << " done: " << done << endl;
					timer.restart();
				}
			}

			output_worker->terminate();
			if (debug>0) outstream << Helper::dateTime() << " analysis finished" << endl;


		}
		catch(...)
		{
			//terminate output worker if an exeption is thrown
			output_worker->terminate();
			throw;
		}
	}
};

#include "main.moc"

int main(int argc, char *argv[])
{
	ConcreteTool tool(argc, argv);
	return tool.execute();
}

