#include "Exceptions.h"
#include "ToolBase.h"
#include "NGSHelper.h"
#include "BamWriter.h"

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
		setDescription("BAM cleaning for Haloplex.");
		addInfile("in", "Input bam file.", false);
		addOutfile("out", "Output bam file.", false);
		//optional
		addInt("min_match", "Minimum number of CIGAR matches (M).", true, 30);
	}

	virtual void main()
	{
		//init
		int min_match = getInt("min_match");
		int c_reads = 0;
		int c_reads_mapped = 0;
		int c_reads_failed = 0;

		BamReader reader(getInfile("in"));
		BamWriter writer(getOutfile("out"));
		writer.writeHeader(reader);

		//process reads
		BamAlignment al;
		while (reader.getNextAlignment(al))
		{
			++c_reads;
			if (!al.isUnmapped() && !al.isSecondaryAlignment() && !al.isDuplicate())
			{
				++c_reads_mapped;
				int sum_m = 0;
				for (CigarOp& op : al.cigarData())
				{
					if (op.Type==BAM_CMATCH) sum_m += op.Length;
				}
				if (sum_m<min_match)
				{
					++c_reads_failed;
					al.setIsUnmapped(true);
					al.setIsSecondaryAlignment(true);
					//QTextStream(stdout) << "REMOVED: " << al.name() << " " << reader.chromosome(al.chromosomeID()).str() << ":" << al.start() << "-" << al.end() << endl;
				}
			}

			writer.writeAlignment(reader, al);
		}

		//statistics output
		QTextStream out(stdout);
		out << "overall reads: " << c_reads << endl;
		out << "mapped reads : " << c_reads_mapped << " (" << QString::number(100.0*c_reads_mapped/c_reads, 'f', 2) << "%)" << endl;
		out << "removed reads: " << c_reads_failed << " (" << QString::number(100.0*c_reads_failed/c_reads, 'f', 2) << "%)" << endl;
	}

};

#include "main.moc"

int main(int argc, char *argv[])
{
	ConcreteTool tool(argc, argv);
	return tool.execute();
}
