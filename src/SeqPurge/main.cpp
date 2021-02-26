#include "ToolBase.h"
#include "AnalysisWorker.h"
#include "OutputWorker.h"
#include "Helper.h"
#include "BasicStatistics.h"
#include <QThreadPool>
#include <QTime>

class ConcreteTool
		: public ToolBase
{
	Q_OBJECT

public:
	ConcreteTool(int& argc, char *argv[])
		: ToolBase(argc, argv)
		, params_()
		, stats_()
	{
	}

	virtual void setup()
	{
		setDescription("Removes adapter sequences from paired-end sequencing data.");
		addInfileList("in1", "Forward input gzipped FASTQ file(s).", false);
		addInfileList("in2", "Reverse input gzipped FASTQ file(s).", false);
		addOutfile("out1", "Forward output gzipped FASTQ file.", false);
		addOutfile("out2", "Reverse output gzipped FASTQ file.", false);
		//optional
		addString("a1", "Forward adapter sequence (at least 15 bases).", true, "AGATCGGAAGAGCACACGTCTGAACTCCAGTCA");
		addString("a2", "Reverse adapter sequence (at least 15 bases).", true, "AGATCGGAAGAGCGTCGTGTAGGGAAAGAGTGT");
		addFloat("match_perc", "Minimum percentage of matching bases for sequence/adapter matches.", true, 80.0);
		addFloat("mep", "Maximum error probability of insert and adapter matches.", true, 0.000001);
		addInt("qcut", "Quality trimming cutoff for trimming from the end of reads using a sliding window approach. Set to 0 to disable.", true, 15);
		addInt("qwin", "Quality trimming window size.", true, 5);
		addInt("qoff", "Quality trimming FASTQ score offset.", true, 33);
		addInt("ncut", "Number of subsequent Ns to trimmed using a sliding window approach from the front of reads. Set to 0 to disable.", true, 7);
		addInt("min_len", "Minimum read length after adapter trimming. Shorter reads are discarded.", true, 30);
		addInt("threads", "The number of threads used for trimming (two additional threads are used for reading and writing).", true, 1);
		addOutfile("out3", "Name prefix of singleton read output files (if only one read of a pair is discarded).", true, false);
		addOutfile("summary", "Write summary/progress to this file instead of STDOUT.", true, true);
		addOutfile("qc", "If set, a read QC file in qcML format is created (just like ReadQC).", true, true);
		addInt("prefetch", "Maximum number of reads that may be pre-fetched into memory to speed up trimming.", true, 1000);
		addFlag("ec", "Enable error-correction of adapter-trimmed reads (only those with insert match).");
		addFlag("debug", "Enables debug output (use only with one thread).");
		addInt("progress", "Enables progress output at the given interval in milliseconds (disabled by default).", true, -1);
		addInt("compression_level", "Output FASTQ compression level from 1 (fastest) to 9 (best compression).", true, Z_BEST_SPEED);

		//changelog
		changeLog(2019, 3, 26, "Added 'compression_level' parameter.");
		changeLog(2019, 2, 11, "Added writer thread to make SeqPurge scale better when using many threads.");
		changeLog(2017, 6, 15, "Changed default value of 'min_len' parameter from 15 to 30.");
		changeLog(2016, 8, 10, "Fixed bug in binomial calculation (issue #1).");
		changeLog(2016, 4, 15, "Removed large part of the overtrimming described in the paper (~75% of reads overtrimmed, ~50% of bases overtrimmed).");
		changeLog(2016, 4,  6, "Added error correction (optional).");
		changeLog(2016, 3, 16, "Version used in the SeqPurge paper: http://bmcbioinformatics.biomedcentral.com/articles/10.1186/s12859-016-1069-7");
	}

	virtual void main()
	{
		//init
		QStringList in1_files = getInfileList("in1");
		QStringList in2_files = getInfileList("in2");
		if (in1_files.count()!=in2_files.count())
		{
			THROW(CommandLineParsingException, "Input file lists 'in1' and 'in2' differ in counts!");
		}

		params_.a1 = getString("a1").trimmed().toLatin1();
		if (params_.a1.count()<15) THROW(CommandLineParsingException, "Forward adapter " + params_.a1 + " too short!");
		params_.a2 = getString("a2").trimmed().toLatin1();
		if (params_.a2.count()<15) THROW(CommandLineParsingException, "Reverse adapter " + params_.a2 + " too short!");
		params_.a_size = std::min(20, std::min(params_.a1.count(), params_.a2.count()));

		params_.match_perc = getFloat("match_perc");
		params_.mep = getFloat("mep");
		params_.min_len = getInt("min_len");
		int prefetch = getInt("prefetch");

		params_.qcut = getInt("qcut");
		params_.qwin = getInt("qwin");
		params_.qoff = getInt("qoff");
		params_.ncut = getInt("ncut");

		params_.qc = getOutfile("qc");
		params_.ec = getFlag("ec");
		params_.debug = getFlag("debug");
		params_.compression_level = getInt("compression_level");

		QSharedPointer<QFile> outfile = Helper::openFileForWriting(getOutfile("summary"), true);
		QTextStream out(outfile.data());
		int progress = getInt("progress");

		//init pre-calculation of factorials
		BasicStatistics::precalculateFactorials();

		//create analysis job pool
		QList<AnalysisJob> job_pool;
		while(job_pool.count() < prefetch)
		{
			job_pool << AnalysisJob();
		}

		//create thread pools
		QThreadPool analysis_pool;
		analysis_pool.setMaxThreadCount(getInt("threads")+1);
		OutputWorker* output_worker = new OutputWorker(job_pool, getOutfile("out1"), getOutfile("out2"), getOutfile("out3"), params_, stats_);
		analysis_pool.start(output_worker);

		try //we need this block to terminate the output_worker in case something goes wrong...
		{
			//process
			QTime timer;
			if (progress>0) timer.start();
			for (int i=0; i<in1_files.count(); ++i)
			{
				if (progress>0) out << Helper::dateTime() << " starting - forward: " << in1_files[i] << " reverse: " << in2_files[i] << endl;

				FastqFileStream in1(in1_files[i], false);
				FastqFileStream in2(in2_files[i], false);
				while (!in1.atEnd() && !in2.atEnd())
				{
					int to_be_analyzed = 0;
					int to_be_written = 0;
					int done = 0;
					for (int j=0; j<job_pool.count(); ++j)
					{
						AnalysisJob& job = job_pool[j];
						switch(job.status)
						{
							case TO_BE_ANALYZED:
								++to_be_analyzed;
								break;
							case TO_BE_WRITTEN:
								++to_be_written;
								break;
							case DONE:
								++done;
								job.clear();
								in1.readEntry(job.e1);
								in2.readEntry(job.e2);
								if (in1.atEnd() || in2.atEnd()) break;
								job.status = TO_BE_ANALYZED;
								analysis_pool.start(new AnalysisWorker(job, params_, stats_, ecstats_));
								break;
							case ERROR: //handle errors during analayis (must be thrown in the main thread)
								THROW(Exception, job.error_message);
						}
					}

					//progress output
					if (progress>0 && timer.elapsed()>progress)
					{
						out << Helper::dateTime() << " progress - to_be_analyzed: " << to_be_analyzed << " to_be_written: " << to_be_written << " done: " << done << endl;
						timer.restart();
					}
				}

				//check that forward and reverse read file are both at the end
				if (!in1.atEnd())
				{
					THROW(FileParseException, "File " + in1_files[i] + " has more entries than " + in2_files[i] + "!");
				}
				if (!in2.atEnd())
				{
					THROW(FileParseException, "File " + in2_files[i] + " has more entries than " + in1_files[i] + "!");
				}
			}

			//close workers and streams
			if (progress>0) out << Helper::dateTime() << " input data read completely - waiting for analysis to finish" << endl;
			int done = 0;
			while(done < job_pool.count())
			{
				done = 0;
				for (int j=0; j<job_pool.count(); ++j)
				{
					AnalysisJob& job = job_pool[j];
					switch(job.status)
					{
						case DONE:
							++done;
							break;
						case ERROR: //handle errors during analayis (must be thrown in the main thread)
							THROW(Exception, job.error_message);
							break;
						default:
							break;
					}
				}

				//progress output
				if (progress>0 && timer.elapsed()>progress)
				{
					out << Helper::dateTime() << " progress - done: " << done << endl;
					timer.restart();
				}
			}
			if (progress>0) out << Helper::dateTime() << " analysis finished" << endl;
			output_worker->terminate();
			delete output_worker; //has to be deleted before the job list > no QScopedPointer is used!

			//print trimming statistics
			if (progress>0) out << Helper::dateTime() << " writing statistics summary" << endl;
			stats_.writeStatistics(out, params_);

			//write qc output file
			if (!params_.qc.isEmpty())
			{
				stats_.qc.getResult().storeToQCML(getOutfile("qc"), QStringList() << in1_files << in2_files, "");
			}

			//print error correction statistics
			if (params_.ec)
			{
				if (progress>0) out << Helper::dateTime() << " writing error corrections summary" << endl;
				ecstats_.writeStatistics(out);
			}
		}
		catch(...)
		{
			output_worker->terminate();
			throw;
		}
	}

private:
	TrimmingParameters params_;
	TrimmingStatistics stats_;
	ErrorCorrectionStatistics ecstats_;
};

#include "main.moc"

int main(int argc, char *argv[])
{
	ConcreteTool tool(argc, argv);
	return tool.execute();
}
