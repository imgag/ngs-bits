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
		addEnum("mode", "Determine if BAM/CRAM contains paired-end or single-end reads (default: paired-end)", true, QStringList() << "paired-end" << "single-end", "paired-end");
		addString("reg", "Export only reads in the given region. Format: chr:start-end.", true);
		addFlag("remove_duplicates", "Does not export duplicate reads into the FASTQ file.");
		addInt("compression_level", "Output FASTQ compression level from 1 (fastest) to 9 (best compression).", true, 1);
		addInt("write_buffer_size", "Output write buffer size (number of FASTQ entry pairs).", true, 100);
		addInfile("ref", "Reference genome for CRAM support (mandatory if CRAM is used).", true);

		changeLog(2020, 11, 27, "Added CRAM support.");
		changeLog(2020,  5, 29, "Massive speed-up by writing in background. Added 'compression_level' parameter.");
		changeLog(2020,  3, 21, "Added 'reg' parameter.");
		changeLog(2023,  3, 22, "Added mode for single-end samples (long reads).");
	}

	static void alignmentToFastq(const QSharedPointer<BamAlignment>& al, FastqEntry& e)
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
	}

	virtual void main()
	{
		//init
		QTime timer;
		timer.start();
		QTextStream out(stdout);
		BamReader reader(getInfile("in"), getInfile("ref"));

		QString out1 = getOutfile("out1");
		QString out2 = getOutfile("out2");
		QString mode = getEnum("mode");

		if (mode == "paired-end" && out2.trimmed().isEmpty()) THROW(ArgumentException, "'out2' has to be provided for paired-end reads!");
		if (mode == "single-end" && out2.trimmed() != "") THROW(ArgumentException, "'out2' cannot be set for single-end reads!");

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






		//create background FASTQ writer
		ReadPairPool pair_pool(write_buffer_size);
		QThreadPool analysis_pool;
		analysis_pool.setMaxThreadCount(1);
		OutputWorker* output_worker;
		if (mode == "paired-end")
		{
			output_worker = new OutputWorker(pair_pool, out1, out2, compression_level);
		}
		else if (mode == "single-end")
		{
			output_worker = new OutputWorker(pair_pool, out1, compression_level);
		}
		else
		{
			THROW(ArgumentException, "Invalid mode '" + mode + "' provided!")
		}
		analysis_pool.start(output_worker);

		long long c_unpaired = 0;
		long long c_paired = 0;
		long long c_duplicates = 0;
		long long c_single_end = 0;
		int max_cached = 0;

		//iterate through reads
		QHash<QByteArray, QSharedPointer<BamAlignment>> al_cache;
		QSharedPointer<BamAlignment> al = QSharedPointer<BamAlignment>(new BamAlignment());
		while (true)
		{
			bool ok = reader.getNextAlignment(*al);
			if (!ok) break;
			//out << al.name() << " PAIRED=" << al.isPaired() << " SEC=" << al.isSecondaryAlignment() << " PROP=" << al.isProperPair() << endl;
			
			//skip secondary alinments
			if(al->isSecondaryAlignment() || al->isSupplementaryAlignment()) continue;

			//skip duplicates
			if (remove_duplicates && al->isDuplicate())
			{
				++c_duplicates;
				continue;
			}

			if(mode == "paired-end")
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
					//out << name << " [AL] First: " << al.isRead1() << " Reverse: " << al.isReverseStrand() << " Seq: " << al.QueryBases.data() << endl;
					//out << name << " [MA] First: " << mate.isRead1() << " Reverse: " << mate.isReverseStrand() << " Seq: " << mate.QueryBases.data() << endl;
					ReadPair& pair = pair_pool.nextFreePair();
					if (al->isRead1())
					{
						alignmentToFastq(al, pair.e1);
						alignmentToFastq(mate, pair.e2);
					}
					else
					{
						alignmentToFastq(mate, pair.e1);
						alignmentToFastq(al, pair.e2);
					}
					pair.status = ReadPair::TO_BE_WRITTEN;
					++c_paired;
				}
				//cache read for later retrieval
				else
				{
					al_cache.insert(name, al);
					al = QSharedPointer<BamAlignment>(new BamAlignment());
				}

				max_cached = std::max(max_cached, al_cache.size());
			}
			else if (mode == "single-end")
			{
				ReadPair& pair = pair_pool.nextFreePair();
				alignmentToFastq(al, pair.e1);
				pair.status = ReadPair::TO_BE_WRITTEN;
				++c_single_end;
			}
			else
			{
				THROW(ArgumentException, "Invalid mode '" + mode + "' provided!")
			}
		}

		//write debug output
		if(mode == "paired-end")
		{
			out << "Pair reads (written)            : " << c_paired << endl;
			out << "Unpaired reads (skipped)        : " << c_unpaired << endl;
			out << "Unmatched paired reads (skipped): " << al_cache.size() << endl;
		}
		else //single-end
		{
			out << "Reads (written)                 : " << c_single_end << endl;
		}

		if (remove_duplicates)
		{
			out << "Duplicate reads (skipped)       : " << c_duplicates << endl;
		}
		out << endl;
		out << "Maximum cached reads            : " << max_cached << endl;
		out << "Time elapsed                    : " << Helper::elapsedTime(timer, true) << endl;

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
