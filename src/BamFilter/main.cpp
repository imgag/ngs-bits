#include "ToolBase.h"
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
		setDescription("Filter alignments in BAM/CRAM file (no input sorting required).");
		addInfile("in", "Input BAM/CRAM file.", false);
		addOutfile("out", "Output BAM/CRAM file.", false);

		addInt("minMQ", "Minimum mapping quality.", true, 30);
		addInt("maxMM", "Maximum number of mismatches in aligned read, -1 to disable.", true, 4);
		addInt("maxGap", "Maximum number of gaps (indels) in aligned read, -1 to disable.", true, 1);
		addInt("minDup", "Minimum number of duplicates.", true, 0);
		addString("ref", "Reference genome for CRAM compression (compulsory for CRAM support).", true);
		addFlag("write_cram", "Writes a CRAM file as output.");

		changeLog(2020,  11, 27, "Added CRAM support.");
	}

	bool alignment_pass(BamAlignment& al) const
	{
		int n_gaps = 0;
		int indel_size = 0;
		QList<CigarOp> cigar = al.cigarData();
		foreach(CigarOp op, cigar)
		{
			if (op.Type == 1 || op.Type == 2)
			{
				indel_size += op.Length; //add length of indel
				++n_gaps; //count +1 for indel
			}
		}

		int n_mismatches = al.tagi("NM") - indel_size;
		int n_duplicates = al.tagi("DP");

		bool filter_gap = maxGap == -1 ? true : n_gaps <= maxGap;
		bool filter_mismatches = maxMM == -1 ? true : n_mismatches <= maxMM;

		return (!al.isUnmapped() &&
				al.isPaired() &&
				!al.isMateUnmapped() &&
				al.mappingQuality() >= minMQ &&
				filter_gap &&
				filter_mismatches &&
				n_duplicates >= minDup
				);
	}

	virtual void main()
	{
		//init
		QTextStream out(stdout);

		int count_pass = 0;
		int count_fail = 0;

		minMQ = getInt("minMQ");
		maxMM = getInt("maxMM");
		maxGap = getInt("maxGap");
		minDup = getInt("minDup");

		BamReader reader(getInfile("in"), getString("ref"));
		BamWriter writer(getOutfile("out"));
		writer.writeHeader(reader);

		//process alignments
		BamAlignment al;
		QHash<QByteArray, BamAlignment> cache; //tracks alignments until mate is seen
		QHash<QByteArray, bool> cache_pass; //tracks pass status of alignments until mate is seen
		while (reader.getNextAlignment(al))
		{
			if(al.isSecondaryAlignment() || al.isSupplementaryAlignment()) continue; //skip secondary/supplementary alignments

			QByteArray name = al.name();

			if (!cache.contains(name))
			{
				//mate note seen

				//add alignment to cache
				cache.insert(name, al);

				//determine pass status
				cache_pass.insert(name, alignment_pass(al));
			}
			else
			{
				//mate seen

				if (cache_pass.value(name) && alignment_pass(al))
				{
					//mate passed, this alignment passes, keep alignments
					writer.writeAlignment(cache.take(name));
					writer.writeAlignment(al);
					cache_pass.remove(name);
					++count_pass;
				}
				else
				{
					//mate and/or this alignment does not pass
					cache.remove(name);
					cache_pass.remove(name);
					++count_fail;
				}
			}

		}

		out << "pairs passed: " << count_pass << endl;
		out << "pairs dropped: " << count_fail << endl;
	}

private:
	int minMQ;
	int maxMM;
	int maxGap;
	int minDup;
};

#include "main.moc"

int main(int argc, char *argv[])
{
	ConcreteTool tool(argc, argv);
	return tool.execute();
}
