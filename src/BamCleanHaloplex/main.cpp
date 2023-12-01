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
		addInfile("in", "Input BAM/CRAM file.", false);
		addOutfile("out", "Output BAM/CRAM file.", false);
		//optional
		addInt("min_match", "Minimum number of CIGAR matches (M).", true, 30);
		addInfile("ref", "Reference genome for CRAM support (mandatory if CRAM is used).", true);

		changeLog(2020,  11, 27, "Added CRAM support.");
	}

	virtual void main()
	{
		//init
		int min_match = getInt("min_match");
		int c_reads = 0;
		int c_reads_mapped = 0;
		int c_reads_failed = 0;

		BamReader reader(getInfile("in"), getInfile("ref"));
		BamWriter writer(getOutfile("out"), getInfile("ref"));
		writer.writeHeader(reader);

		//process reads
		BamAlignment al;
		while (reader.getNextAlignment(al))
		{
			++c_reads;
			if (!al.isUnmapped() && !al.isSecondaryAlignment() && !al.isSupplementaryAlignment() && !al.isDuplicate())
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

			writer.writeAlignment(al);
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
