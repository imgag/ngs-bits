#include "ToolBase.h"
#include "FastqFileStream.h"
#include "NGSHelper.h"
#include "Helper.h"
#include <QThreadPool>
#include "OutputWorker.h"

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
		setDescription("Converts a coordinate-sorted BAM file to FASTQ files.");
		addInfile("in", "Input BAM/CRAM file.", false, true);
		addOutfile("out1", "Read 1 output FASTQ.GZ file.", false);

		//optional
		addOutfile("out2", "Read 2 output FASTQ.GZ file (required for pair-end samples).", true);
		addString("reg", "Export only reads in the given region. Format: chr:start-end.", true);
		addFlag("remove_duplicates", "Does not export reads marked as duplicates in SAM flags into the FASTQ file.");
		addInt("compression_level", "Output FASTQ compression level from 1 (fastest) to 9 (best compression).", true, 1);
		addInt("write_buffer_size", "Output write buffer size (number of FASTQ entry pairs).", true, 100);
		addInfile("ref", "Reference genome for CRAM support (mandatory if CRAM is used).", true);
		addInt("extend", "Extend all reads to the given length. Base 'N' and base qualiy '2' are used for extension.", true, 0);
		addFlag("fix", "Keep only one read pair if several have the same name (note: needs much memory as read names are kept in memory).");

		changeLog(2024, 12, 13, "Added 'fix' parameter.");
		changeLog(2024, 12,  9, "Added 'extend' parameter.");
		changeLog(2020, 11, 27, "Added CRAM support.");
		changeLog(2020,  5, 29, "Massive speed-up by writing in background. Added 'compression_level' parameter.");
		changeLog(2020,  3, 21, "Added 'reg' parameter.");
		changeLog(2023,  3, 22, "Added mode for single-end samples (long reads).");
	}

	static void alignmentToFastq(const QSharedPointer<BamAlignment>& al, FastqEntry& e, int extend)
	{
		e.header = "@" + al->name();
		e.bases = al->bases();
		e.header2 = "+";
		e.qualities = al->qualities();

		if (al->isReverseStrand())
		{
			e.bases.reverseComplement();
			std::reverse(e.qualities.begin(), e.qualities.end());
		}

		if (extend>0)
		{
            while (e.bases.size()<extend)
			{
				e.bases.append('N');
				e.qualities.append(35); //33+2
			}
		}
	}

	struct ReadWritten
	{
		bool read1 = false;
		bool read2 = false;

		bool done(bool first_read) const
		{
			return first_read ? read1 : read2;
		}
		void setDone(bool first_read)
		{
			if(first_read) read1 = true;
			else read2 = true;
		}
	};

	virtual void main()
	{
		//init
        QElapsedTimer timer;
		timer.start();
		QTextStream out(stdout);
		BamReader reader(getInfile("in"), getInfile("ref"));
		reader.skipTags();
		QString out1 = getOutfile("out1");
		QString out2 = getOutfile("out2");
		bool fix = getFlag("fix");
		QString reg = getString("reg");
		if (reg!="")
		{
			BedLine region = BedLine::fromString(reg);
			if (!region.isValid())
			{
				THROW(CommandLineParsingException, "Given region '" + reg + "' is not valid!");
			}
			reader.setRegion(region.chr(), region.start(), region.end());
		}
		bool remove_duplicates = getFlag("remove_duplicates");
		int write_buffer_size = getInt("write_buffer_size");

		int compression_level = getInt("compression_level");
		int extend = getInt("extend");

		//create background FASTQ writer
		ReadPairPool pair_pool(write_buffer_size);
		QThreadPool analysis_pool;
		analysis_pool.setMaxThreadCount(1);
		OutputWorker* output_worker;
		const bool is_pe = !out2.trimmed().isEmpty();
		if (is_pe)
		{
			output_worker = new OutputWorker(pair_pool, out1, out2, compression_level);
		}
		else //single-end
		{
			output_worker = new OutputWorker(pair_pool, out1, compression_level);
		}
		analysis_pool.start(output_worker);

		long long c_unpaired = 0;
		long long c_paired = 0;
		long long c_duplicates = 0;
		long long c_single_end = 0;
		long long c_fixed = 0;
		int max_cached = 0;
		//iterate through reads
		QHash<QByteArray, QSharedPointer<BamAlignment>> al_cache;
		QHash<QByteArray, ReadWritten> reads_processed;
		QSharedPointer<BamAlignment> al = QSharedPointer<BamAlignment>(new BamAlignment());
		while (reader.getNextAlignment(*al))
		{
			//skip secondary alinments
			if(al->isSecondaryAlignment() || al->isSupplementaryAlignment()) continue;

			//skip duplicates
			if (remove_duplicates && al->isDuplicate())
			{
				++c_duplicates;
				continue;
			}

			//remove reads with same name (but handle read pairing properly)
			if (fix)
			{
				ReadWritten& written = reads_processed[al->name()];
				if (written.done(al->isRead1()))
				{
					++c_fixed;
					continue;
				}
				written.setDone(al->isRead1());
			}

			if(is_pe)
			{
				//skip unpaired
				if(!al->isPaired())
				{
					++c_unpaired;
					continue;
				}

				//store cached read when we encounter the mate
				QByteArray name = al->name();
				if (al_cache.contains(name))
				{
					QSharedPointer<BamAlignment> mate = al_cache.take(name);					
					ReadPair& pair = pair_pool.nextFreePair();
					if (al->isRead1())
					{
						alignmentToFastq(al, pair.e1, extend);
						alignmentToFastq(mate, pair.e2, extend);
					}
					else
					{
						alignmentToFastq(mate, pair.e1, extend);
						alignmentToFastq(al, pair.e2, extend);
					}
					pair.status = ReadPair::TO_BE_WRITTEN;
					++c_paired;
				}
				else //cache read for later retrieval
				{
					al_cache.insert(name, al);
					al = QSharedPointer<BamAlignment>(new BamAlignment());
				}

                max_cached = std::max(SIZE_TO_INT(max_cached), SIZE_TO_INT(al_cache.size()));
			}
			else //single-end
			{
				ReadPair& pair = pair_pool.nextFreePair();
				alignmentToFastq(al, pair.e1, extend);
				pair.status = ReadPair::TO_BE_WRITTEN;
				++c_single_end;
			}
		}

		//write debug output
		if(is_pe)
		{
            out << "Pair reads (written)            : " << c_paired << QT_ENDL;
            out << "Unpaired reads (skipped)        : " << c_unpaired << QT_ENDL;
            out << "Unmatched paired reads (skipped): " << al_cache.size() << QT_ENDL;
		}
		else //single-end
		{
            out << "Reads (written)                 : " << c_single_end << QT_ENDL;
		}
		if (remove_duplicates)
		{
            out << "Duplicate tagged reads (skipped): " << c_duplicates << QT_ENDL;
		}
		if (fix)
		{
            out << "Duplicate name reads (skipped)  : " << c_fixed << QT_ENDL;
		}
        out << QT_ENDL;
        out << "Maximum cached reads            : " << max_cached << QT_ENDL;
        out << "Time elapsed                    : " << Helper::elapsedTime(timer, true) << QT_ENDL;

		//terminate FASTQ writer after all reads are written
		pair_pool.waitAllWritten();
		output_worker->terminate();
		delete output_worker; //has to be deleted before the read pair list > no QScopedPointer is used!
	}
};

#include "main.moc"

int main(int argc, char *argv[])
{
	ConcreteTool tool(argc, argv);
	return tool.execute();
}
