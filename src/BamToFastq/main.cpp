#include "ToolBase.h"
#include "FastqFileStream.h"
#include "NGSHelper.h"
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
		setDescription("Converts a coordinate-sorted BAM file to FASTQ files (paired-end only).");
		addInfile("in", "Input BAM file.", false, true);
		addOutfile("out1", "Read 1 output FASTQ.GZ file.", false);
		addOutfile("out2", "Read 2 output FASTQ.GZ file.", false);
		//optional
		addString("reg", "Export only reads in the given region. Format: chr:start-end.", true);
		addFlag("remove_duplicates", "Does not export duplicate reads into the FASTQ file.");

		changeLog(2020,  3, 21, "Added 'reg' parameter.");
	}

	void write(FastqOutfileStream& out, const QSharedPointer<BamAlignment>& al)
	{
		//create FASTQ entry
		FastqEntry e;
		e.header = "@" + al->name();
		e.bases = al->bases();
		e.header2 = "+";
		e.qualities = al->qualities();

		if (al->isReverseStrand())
		{
			e.bases.reverseComplement();
			std::reverse(e.qualities.begin(), e.qualities.end());
		}

		out.write(e);
	}

	virtual void main()
	{
		//init
		QTime timer;
		timer.start();
		QTextStream out(stdout);
		BamReader reader(getInfile("in"));
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

		FastqOutfileStream out1(getOutfile("out1"));
		FastqOutfileStream out2(getOutfile("out2"));

		long long c_unpaired = 0;
		long long c_paired = 0;
		long long c_duplicates = 0;
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
			if(al->isSecondaryAlignment()) continue;

			//skip duplicates
			if (remove_duplicates && al->isDuplicate())
			{
				++c_duplicates;
				continue;
			}
			
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
				if (al->isRead1())
				{
					write(out1, al);
					write(out2, mate);
				}
				else
				{
					write(out1, mate);
					write(out2, al);
				}
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
		out1.close();
		out2.close();

		//write debug output
		out << "Pair reads (written)            : " << c_paired << endl;
		out << "Unpaired reads (skipped)        : " << c_unpaired << endl;
		out << "Unmatched paired reads (skipped): " << al_cache.size() << endl;
		if (remove_duplicates)
		{
			out << "Duplicate reads (skipped)       : " << c_duplicates << endl;
		}
		out << endl;
		out << "Maximum cached reads            : " << max_cached << endl;
		out << "Time elapsed: " << Helper::elapsedTime(timer, true) << endl;
	}
};
#include "main.moc"

int main(int argc, char *argv[])
{
	ConcreteTool tool(argc, argv);
	return tool.execute();
}
