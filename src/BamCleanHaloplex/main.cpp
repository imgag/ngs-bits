#include "Exceptions.h"
#include "ToolBase.h"
#include "NGSHelper.h"
#include "api/BamReader.h"
#include "api/BamWriter.h"

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
		setDescription("BAM cleaning for Haloplex.");
		addInfile("in", "Input bam file.", false);
		addOutfile("out", "Output bam file.", false);
		//optinal
		addInt("min_match", "Minimum number of CIGAR matches (M).", true, 30);
	}

	virtual void main()
	{
		//init
		int min_match = getInt("min_match");
		int c_reads = 0;
		int c_reads_mapped = 0;
		int c_reads_failed = 0;

		BamReader reader;
		NGSHelper::openBAM(reader, getInfile("in"));
		BamWriter writer;
		writer.Open(getOutfile("out").toStdString(), reader.GetConstSamHeader(), reader.GetReferenceData());

		//process reads
		BamAlignment al;
		while (reader.GetNextAlignment(al))
		{
			++c_reads;
			if (al.IsMapped() && al.IsPrimaryAlignment() && !al.IsDuplicate())
			{
				++c_reads_mapped;
				int sum_m = 0;
				int sum_s = 0;
				for (CigarOp& op : al.CigarData)
				{
					if (op.Type=='M') sum_m += op.Length;
					if (op.Type=='S') sum_s += op.Length;
				}
				if (sum_m<min_match)
				{
					++c_reads_failed;
					al.SetIsMapped(false);
					al.SetIsPrimaryAlignment(false);
					//qDebug() << "FAIL" << al.Name.c_str() << al.Length << sum_m << sum_s << al.QueryBases.c_str() << reader.GetReferenceData()[al.RefID].RefName.c_str() << al.Position;
				}
			}

			writer.SaveAlignment(al);
		}
		reader.Close();
		writer.Close();

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
