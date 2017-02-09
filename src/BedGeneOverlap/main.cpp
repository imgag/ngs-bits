#include "BedFile.h"
#include "ToolBase.h"
#include "ChromosomalIndex.h"
#include "Exceptions.h"
#include "Helper.h"
#include "Settings.h"
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
		sources << "ccds" << "refseq" << "ucsc";
		addEnum("source", "Transcript source database.", false, sources);
		addOutfile("out", "Output TSV file. If unset, writes to STDOUT.", true);
		addFlag("test", "Uses the test database instead of on the production database.");

		changeLog(2017,  2,  9, "Added RefSeq source.");
	}

	virtual void main()
	{
		//init
		NGSD db(getFlag("test"));

		//load input file
		BedFile in;
		in.load(getInfile("in"));
		in.merge();

		//look up genes overlapping the input region
		QStringList genes;
		for(int i=0; i<in.count(); ++i)
		{
			BedLine& line = in[i];
			genes << db.genesOverlapping(line.chr(), line.start(), line.end(), 0);
		}
		genes.removeDuplicates();

		//output header
		QStringList output;
		output.append("#gene\tsize\toverlap\tpercentage");

		//process
		BedFile reg_unassigned = in;
		foreach(QString gene, genes)
		{
			//create gene-specific regions
			QTextStream messages(stderr);
			BedFile reg_gene = db.genesToRegions(QStringList() << gene, Transcript::stringToSource(getEnum("source")), "exon", false, false, &messages);
			reg_gene.merge();

			//append output line
            long long bases_gene = reg_gene.baseCount();
			if (bases_gene==0) continue; //non-coding gene => skip
			reg_gene.intersect(in);
            long long bases_covered = reg_gene.baseCount();
			if (bases_covered==0) continue; //wrong coding range in UCSC => gene does not really overlap => skip

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
