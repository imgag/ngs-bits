#include "ToolBase.h"
#include "BamWriter.h"
#include "TabixIndexedFile.h"

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
		setDescription("Removes reads which contain the provided variants");
		addInfile("in", "Input BAM/CRAM file.", false);
		addOutfile("out", "Output BAM/CRAM file.", false);
		addInfile("vcf", "Input indexed VCF.GZ file.", false);

		addInfile("ref", "Reference genome for CRAM support (mandatory if CRAM is used).", true);

		changeLog(2024, 7, 24, "Inital commit.");
	}

	bool alignment_pass(BamAlignment& al, const BamReader& reader) const
	{
		//get variants
		QByteArrayList matches = vcf.getMatchingLines(reader.chromosome(al.chromosomeID()), al.start(), al.end(), true);

		if (matches.count() == 0) return true; //no overlapping variants

		foreach (const QByteArray& match, matches)
		{
			QByteArrayList columns = match.split('\t');
			VcfLine vcf_line(Chromosome(columns.at(0)), columns.at(1).toInt(), columns.at(3), QList<Sequence>() << columns.at(4));
			Variant var(vcf_line);

			if (var.isSNV())
			{
				QPair<char, int> base = al.extractBaseByCIGAR(var.start());
				Sequence read_base;
				read_base.append(base.first);

				if (read_base == var.obs()) return false;
				//special handling of undefined ID observed base:
				if ((var.obs() == "<NON_REF>") && (read_base != var.ref())) return false;
			}
			else //InDel
			{
				//remove all reads which has an InDel at this position.
				QList<Sequence> indels = al.extractIndelsByCIGAR(var.start(), 50);
				if (indels.size() > 0) return false;
			}
		}

		return true;
	}

	virtual void main()
	{
		//init
		QTextStream out(stdout);

		int count_pass = 0;
		int count_fail = 0;


		BamReader reader(getInfile("in"), getInfile("ref"));
		BamWriter writer(getOutfile("out"), getInfile("ref"));
		writer.writeHeader(reader);

		//read VCF file:
		vcf.load(getInfile("vcf").toUtf8());

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
				cache_pass.insert(name, alignment_pass(al, reader));
			}
			else
			{
				//mate seen

				if (cache_pass.value(name) && alignment_pass(al, reader))
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
	TabixIndexedFile vcf;

};

#include "main.moc"

int main(int argc, char *argv[])
{
	ConcreteTool tool(argc, argv);
	return tool.execute();
}
