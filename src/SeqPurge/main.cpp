#include "ToolBase.h"
#include "ThreadCoordinator.h"
#include "AnalysisWorker.h"
#include "OutputWorker.h"
#include "Helper.h"
#include "BasicStatistics.h"
#include <QThreadPool>

class ConcreteTool
		: public ToolBase
{
	Q_OBJECT

public:
	ConcreteTool(int& argc, char *argv[])
		: ToolBase(argc, argv)
	{
		setExitEventLoopAfterMain(false);
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
		addInt("threads", "The number of threads used for trimming (up to three additional threads are used for reading and writing).", true, 1);
		addOutfile("out3", "Name prefix of singleton read output files (if only one read of a pair is discarded).", true, false);
		addOutfile("summary", "Write summary/progress to this file instead of STDOUT.", true, true);
		addOutfile("qc", "If set, a read QC file in qcML format is created (just like ReadQC).", true, true);
		addInt("block_size", "Number of FASTQ entries processed in one block.", true, 10000);
		addInt("block_prefetch", "Number of blocks that may be pre-fetched into memory.", true, 32);
		addFlag("ec", "Enable error-correction of adapter-trimmed reads (only those with insert match).");
		addFlag("debug", "Enables debug output (use only with one thread).");
		addInt("progress", "Enables progress output at the given interval in milliseconds (disabled by default).", true, -1);
		addInt("compression_level", "Output FASTQ compression level from 1 (fastest) to 9 (best compression).", true, Z_BEST_SPEED);

		//changelog
		changeLog(2022, 7, 15, "Improved scaling with more than 4 threads and CPU usage.");
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
		//load parameters
		TrimmingParameters params;
		params.files_in1 = getInfileList("in1");
		params.files_in2 = getInfileList("in2");
		params.out1 = getOutfile("out1");
		params.out2 = getOutfile("out2");
		params.out3 = getOutfile("out3");
		params.summary = getOutfile("summary");
		if (params.files_in1.count()!=params.files_in2.count()) THROW(CommandLineParsingException, "Input file lists 'in1' and 'in2' differ in counts!");
		params.a1 = getString("a1").trimmed().toUtf8();
		if (params.a1.count()<15) THROW(CommandLineParsingException, "Forward adapter " + params.a1 + " too short!");
		params.a2 = getString("a2").trimmed().toUtf8();
		if (params.a2.count()<15) THROW(CommandLineParsingException, "Reverse adapter " + params.a2 + " too short!");
		params.a_size = std::min(20, std::min(params.a1.count(), params.a2.count()));

		params.match_perc = getFloat("match_perc");
		params.mep = getFloat("mep");
		params.min_len = getInt("min_len");
		params.block_prefetch = getInt("block_prefetch");
		params.block_size = getInt("block_size");
		params.threads = getInt("threads");
		params.progress = getInt("progress");

		params.qcut = getInt("qcut");
		params.qwin = getInt("qwin");
		params.qoff = getInt("qoff");
		params.ncut = getInt("ncut");

		params.qc = getOutfile("qc");
		params.ec = getFlag("ec");
		params.debug = getFlag("debug");
		params.compression_level = getInt("compression_level");

		//init pre-calculation of factorials
		BasicStatistics::precalculateFactorials();

		//create cooridinator instance (it starts the processing)
		ThreadCoordinator* coordinator = new ThreadCoordinator(this, params);
		connect(coordinator, SIGNAL(finished()), QCoreApplication::instance(), SLOT(quit()));
	}

};

#include "main.moc"

int main(int argc, char *argv[])
{
	ConcreteTool tool(argc, argv);
	return tool.execute();
}
