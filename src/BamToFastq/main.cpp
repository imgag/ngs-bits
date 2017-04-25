#include "ToolBase.h"
#include "api/BamReader.h"
#include "api/BamWriter.h"
#include "api/BamAlgorithms.h"
#include "FastqFileStream.h"
#include "NGSHelper.h"

using namespace BamTools;

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
	}

	void write(FastqOutfileStream& out, const BamAlignment& al, bool rev_comp)
	{
		//create FASTQ entry
		FastqEntry e;
		e.header = "@" + QByteArray::fromRawData(al.Name.data(), al.Name.size());
		e.bases = QByteArray::fromRawData(al.QueryBases.data(), al.QueryBases.size());
		e.header2 = "+";
		e.qualities = QByteArray::fromRawData(al.Qualities.data(), al.Qualities.size());

		if (rev_comp)
		{
			e.bases = NGSHelper::changeSeq(e.bases, true, true);
			e.qualities = NGSHelper::changeSeq(e.qualities, true, false);
		}

		out.write(e);
	}

	virtual void main()
	{
		//init
		QTextStream out(stdout);
		BamReader reader;
		NGSHelper::openBAM(reader, getInfile("in"));

		FastqOutfileStream out1(getOutfile("out1"), false);
		FastqOutfileStream out2(getOutfile("out2"), false);

		long long c_unpaired = 0;
		long long c_paired = 0;
		int max_cached = 0;

		//iterate through reads
		BamAlignment al;
		QHash<QByteArray, BamAlignment> al_cache;
		while (reader.GetNextAlignment(al))
		{
			//skip secondary alinments
			if(!al.IsPrimaryAlignment()) continue;

			//skip unpaired
			if(!al.IsPaired())
			{
				++c_unpaired;
				continue;
			}

			QByteArray name = QByteArray::fromStdString(al.Name);

			//store cached read when we encounter the mate
			if (al_cache.contains(name))
			{
				BamAlignment mate = al_cache.take(name);
				//out << name << " [AL] First: " << al.IsFirstMate() << " Reverse: " << al.IsReverseStrand() << " Seq: " << al.QueryBases.data() << endl;
				//out << name << " [MA] First: " << mate.IsFirstMate() << " Reverse: " << mate.IsReverseStrand() << " Seq: " << mate.QueryBases.data() << endl;
				if (al.IsFirstMate())
				{
					write(out1, al, al.IsReverseStrand());
					write(out2, mate, mate.IsReverseStrand());
				}
				else
				{
					write(out1, mate, mate.IsReverseStrand());
					write(out2, al, al.IsReverseStrand());
				}
				++c_paired;
			}
			//cache read for later retrieval
			else
			{
				al_cache.insert(name, al);
			}

			max_cached = std::max(max_cached, al_cache.size());
		}
		reader.Close();
		out1.close();
		out2.close();

		//write debug output
		out << "Pair reads (written)            : " << c_paired << endl;
		out << "Unpaired reads (skipped)        : " << c_unpaired << endl;
		out << "Unmatched paired reads (skipped): " << al_cache.size() << endl;
		out << endl;
		out << "Maximum cached reads            : " << max_cached << endl;
	}
};
#include "main.moc"

int main(int argc, char *argv[])
{
	ConcreteTool tool(argc, argv);
	return tool.execute();
}
