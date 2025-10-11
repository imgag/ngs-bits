#include "BedFile.h"
#include "ToolBase.h"
#include "Helper.h"
#include "NGSD.h"

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
		setDescription("Calculates how much of each overlapping gene is covered.");
		addInfile("in", "Input BED file. If unset, reads from STDIN.", true);
		QStringList sources;
		sources << "ccds" << "ensembl";
		addEnum("source", "Transcript source database.", false, sources);
		addOutfile("out", "Output TSV file. If unset, writes to STDOUT.", true);
		addFlag("test", "Uses the test database instead of on the production database.");
	}

	virtual void main()
	{
		//init
		NGSD db(getFlag("test"));
		Transcript::SOURCE source = Transcript::stringToSource(getEnum("source"));

		//load input file
		BedFile in;
		in.load(getInfile("in"));
		in.merge();

		//look up genes overlapping the input region
		GeneSet genes;
		for(int i=0; i<in.count(); ++i)
		{
			genes.insert(db.genesOverlapping(in[i].chr(), in[i].start(), in[i].end(), 0));
		}

		//output header
		QStringList output;
		output.append("#gene\tsize\toverlap\tpercentage");

		//process
		BedFile reg_unassigned = in;
        for (const QByteArray& gene : genes)
		{
			//create gene-specific regions
			QTextStream messages(stderr);
			BedFile reg_gene = db.genesToRegions(GeneSet() << gene, source, "exon", false, false, &messages);
			reg_gene.merge();

			//append output line
			long long bases_gene = reg_gene.baseCount();
			reg_gene.intersect(in);
			long long bases_covered = reg_gene.baseCount();

			output.append(gene + "\t" + QString::number(bases_gene) + "\t" + QString::number(bases_covered) + "\t" + QString::number(100.0*bases_covered/bases_gene, 'f', 2));

			//update unassigned regions
			reg_gene.sort();
			reg_unassigned.subtract(reg_gene);
		}

		//append region size that is not assigned to a gene
		output.append("none\tn/a\t" + QString::number(reg_unassigned.baseCount()) + "\tn/a");

		//store output
		output.sort();
		Helper::storeTextFile(Helper::openFileForWriting(getOutfile("out"), true), output);
	}
};

#include "main.moc"

int main(int argc, char *argv[])
{
	ConcreteTool tool(argc, argv);
	return tool.execute();
}
