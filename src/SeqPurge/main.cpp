#include "ToolBase.h"
#include "Auxilary.h"
#include "AnalysisWorker.h"
#include "Helper.h"
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
		, data_()
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
		addInt("threads", "The number of threads used for trimming (an additional thread is used for reading data).", true, 1);
		addOutfile("out3", "Name prefix of singleton read output files (if only one read of a pair is discarded).", true, false);
		addOutfile("summary", "Write summary/progress to this file instead of STDOUT.", true, true);
		addOutfile("qc", "If set, a read QC file in qcML format is created (just like ReadQC).", true, true);
		addInt("prefetch", "Maximum number of reads that may be pre-fetched to speed up trimming", true, 1000);
		addFlag("ec", "Enable error-correction of adapter-trimmed reads (only those with insert match).");
		addFlag("debug", "Enables debug output (use only with one thread).");
		addFlag("progress", "Enables progress output.");

		//changelog
		changeLog(2017, 6, 15, "Changed default value of 'min_len' parameter from 15 to 30.");
		changeLog(2016, 8, 10, "Fixed bug in binomial calculation (issue #1).");
		changeLog(2016, 4, 15, "Removed large part of the overtrimming described in the paper (~75% of reads overtrimmed, ~50% of bases overtrimmed).");
		changeLog(2016, 4,  6, "Added error correction (optional).");
		changeLog(2016, 3, 16, "Version used in the SeqPurge paper: http://bmcbioinformatics.biomedcentral.com/articles/10.1186/s12859-016-1069-7");
	}

	int processingReadPairs() const
	{
		return data_.reads_queued - stats_.read_num;
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

		data_.out1 = new FastqOutfileStream(getOutfile("out1"), false);
		data_.out2 = new FastqOutfileStream(getOutfile("out2"), false);

		params_.a1 = getString("a1").trimmed().toLatin1();
		if (params_.a1.count()<15) THROW(CommandLineParsingException, "Forward adapter " + params_.a1 + " too short!");
		params_.a2 = getString("a2").trimmed().toLatin1();
		if (params_.a2.count()<15) THROW(CommandLineParsingException, "Reverse adapter " + params_.a2 + " too short!");
		params_.a_size = std::min(20, std::min(params_.a1.count(), params_.a2.count()));

		params_.match_perc = getFloat("match_perc");
		params_.mep = getFloat("mep");
		params_.min_len = getInt("min_len");
		params_.max_reads_queued = getInt("prefetch");

		params_.qcut = getInt("qcut");
		params_.qwin = getInt("qwin");
		params_.qoff = getInt("qoff");
		params_.ncut = getInt("ncut");

		QString out3_base = getOutfile("out3").trimmed();
		if (out3_base!="")
		{
			data_.out3 = new FastqOutfileStream(out3_base + "_R1.fastq.gz", false);
			data_.out4 = new FastqOutfileStream(out3_base + "_R2.fastq.gz", false);
		}

		params_.qc = getOutfile("qc");
		data_.analysis_pool.setMaxThreadCount(getInt("threads"));
		params_.ec = getFlag("ec");
		params_.debug = getFlag("debug");

		QSharedPointer<QFile> outfile = Helper::openFileForWriting(getOutfile("summary"), true);
		QTextStream out(outfile.data());
		bool progress = getFlag("progress");
		QTime timer;
		if (progress) timer.start();

		//init pre-calculation of faktorials
		AnalysisWorker::precalculateFactorials();

		//process
		for (int i=0; i<in1_files.count(); ++i)
		{
			if (progress) out << Helper::dateTime() << " starting - forward: " << in1_files[i] << " reverse: " << in2_files[i] << endl;

			FastqFileStream in1(in1_files[i], false);
			FastqFileStream in2(in2_files[i], false);
			while (!in1.atEnd() && !in2.atEnd())
			{
				//prevent caching of too many reads (waste of memory)
				while(processingReadPairs()>=params_.max_reads_queued)
				{
					QThread::msleep(1);

					if (progress && timer.elapsed()>10000)
					{
						out << Helper::dateTime() << " waiting - processing now: " << processingReadPairs() << " total processed: " << data_.reads_queued << endl;
						timer.restart();
					}
				}

				if (progress && timer.elapsed()>10000)
				{
					out << Helper::dateTime() << " reading - processing now: " << processingReadPairs() << " total processed: " << data_.reads_queued << endl;
					timer.restart();
				}
				data_.reads_queued += 2;

				QSharedPointer<FastqEntry> e1(new FastqEntry());
				in1.readEntry(*e1);
				QSharedPointer<FastqEntry> e2(new FastqEntry());
				in2.readEntry(*e2);

				data_.analysis_pool.start(new AnalysisWorker(e1, e2, params_, stats_, ecstats_, data_));
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

		//close streams
		if (progress) out << Helper::dateTime() << " closing - processing now: " << processingReadPairs() << " total processed: " << data_.reads_queued << endl;
		data_.closeOutStreams();
		if (progress) out << Helper::dateTime() << " closed - processing now: " << processingReadPairs() << " total processed: " << data_.reads_queued << endl;

		//print trimming statistics
		if (progress) out << Helper::dateTime() << " writing statistics summary" << endl;
		stats_.writeStatistics(out, params_);

		//write qc output file
		if (!params_.qc.isEmpty())
		{
			stats_.qc.getResult().storeToQCML(getOutfile("qc"), QStringList() << in1_files << in2_files, "");
		}

		//print error correction statistics
		if (params_.ec)
		{
			if (progress) out << Helper::dateTime() << " writing error corrections summary" << endl;
			ecstats_.writeStatistics(out);
		}
	}

private:
	TrimmingParameters params_;
	TrimmingStatistics stats_;
	ErrorCorrectionStatistics ecstats_;
	TrimmingData data_;
};

#include "main.moc"

int main(int argc, char *argv[])
{
	ConcreteTool tool(argc, argv);
	return tool.execute();
}


/* test code with bit-arrays (non-dynamic)
//create sequences
int repeats = 100000;
int seq_len = 100;

QVector<QByteArray> seqs;
for (int i=0; i<=repeats; ++i)
{
    QByteArray seq = Helper::randomString(seq_len, "ACGT").toLatin1();
    seq[95] = 'N';
    seq[97] = 'N';
    seq[99] = 'N';
    seqs.append(seq);
}

//classical match calculation
int match_sum = 0;
int mismatch_sum = 0;
int invalid_sum = 0;
QTime timer;
timer.start();
for (int i=0; i<repeats; ++i)
{
    const char* seq1_data = seqs[i].constData();
    const char* seq2_data = seqs[i+1].constData();
    for (int offset=1; offset<seq_len; ++offset)
    {
        int matches = 0;
        int mismatches = 0;
        int invalid = 0;
        for (int j=offset; j<seq_len; ++j)
        {
            char b1 = seq1_data[j-offset];
            char b2 = seq2_data[j];
            if (b1=='N' || b2=='N')
            {
                ++invalid;
            }
            else if (b1==b2)
            {
                ++matches;
            }
            else
            {
                ++mismatches;
            }
        }
        match_sum += matches;
        mismatch_sum += mismatches;
        invalid_sum += invalid;
    }
}
qDebug() << "old" << match_sum << mismatch_sum << invalid_sum << " timer:" << timer.elapsed();

//bit-array-based match calculation
match_sum = 0;
mismatch_sum = 0;
invalid_sum = 0;
timer.restart();
for (int i=0; i<repeats; ++i)
{
    const QByteArray& seq1 = seqs[i];
    std::bitset<100> a1_;
    std::bitset<100> c1_;
    std::bitset<100> g1_;
    std::bitset<100> t1_;
    std::bitset<100> n1_;
    const QByteArray& seq2 = seqs[i+1];
    std::bitset<100> a2_;
    std::bitset<100> c2_;
    std::bitset<100> g2_;
    std::bitset<100> t2_;
    std::bitset<100> n2_;

    for (int i=0; i<seq1.size(); ++i)
    {
        const int i_r = seq1.size()-i-1;
        switch(seq1[i_r])
        {
            case 'C':
                c1_.set(i, true);
                break;
            case 'A':
                a1_.set(i, true);
                break;
            case 'G':
                g1_.set(i, true);
                break;
            case 'T':
                t1_.set(i, true);
                break;
            case 'N':
                n1_.set(i, true);
                break;
            default:
                THROW(ArgumentException, QString("Unknown base '") + seq1[i] + "'!");
        }

        switch(seq2[i_r])
        {
            case 'C':
                c2_.set(i, true);
                break;
            case 'A':
                a2_.set(i, true);
                break;
            case 'G':
                g2_.set(i, true);
                break;
            case 'T':
                t2_.set(i, true);
                break;
            case 'N':
                n2_.set(i, true);
                break;
            default:
                THROW(ArgumentException, QString("Unknown base '") + seq2[i] + "'!");
        }
    }

    for (int offset=1; offset<seq_len; ++offset)
    {

        a2_ <<= 1;
        c2_ <<= 1;
        g2_ <<= 1;
        t2_ <<= 1;
        n2_ <<= 1;
        n1_.set(offset-1, false);
        int invalid = (n1_ | n2_).count();
        int matches = (a1_ & a2_).count() + (c1_ & c2_).count() + (g1_ & g2_).count() + (t1_ & t2_).count();
        int mismatches = seq_len - offset - invalid - matches;

        match_sum += matches;
        mismatch_sum += mismatches;
        invalid_sum += invalid;
    }
}
qDebug() << "new" << match_sum << mismatch_sum << invalid_sum << " timer:" << timer.elapsed();
*/
