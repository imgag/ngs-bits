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
		setDescription("Converts a BAM file to FASTQ files (paired-end only).");
		addInfile("in", "Input BAM file.", false, true);		
		addOutfile("out1", "Read 1 output FASTQ.GZ file.", false);
		addOutfile("out2", "Read 2 output FASTQ.GZ file.", false);
	}

	void write(FastqOutfileStream& out, const BamAlignment& al)
	{
		FastqEntry e;
		e.header = "@" + QByteArray::fromRawData(al.Name.data(), al.Name.size());
		e.bases = QByteArray::fromRawData(al.QueryBases.data(), al.QueryBases.size());
		e.header2 = "+";
		e.qualities = QByteArray::fromRawData(al.Qualities.data(), al.Qualities.size());
		out.write(e);
	}

	virtual void main()
	{
		//init
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
				//TODO handle pair orientation!
				write(out1, mate);
				write(out2, al);
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
		QTextStream out(stdout);
		out << "Pair reads (written)     : " << c_paired << endl;
		out << "Unpaired reads (skipped) : " << c_unpaired << endl;
		out << endl;
		out << "Maximum cached alignments: " << max_cached << endl;
	}
};
#include "main.moc"

int main(int argc, char *argv[])
{
	ConcreteTool tool(argc, argv);
	return tool.execute();
}
