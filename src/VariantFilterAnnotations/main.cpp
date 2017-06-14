#include "Exceptions.h"
#include "ToolBase.h"
#include "ChromosomalIndex.h"
#include "VariantList.h"
#include "BedFile.h"
#include "Helper.h"
#include "VariantFilter.h"

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
		setDescription("Filter a variant list in GSvar format based on variant annotations.");
		addInfile("in", "Input variant list in GSvar format.", false);
		addOutfile("out", "Output variant list. If unset, writes to STDOUT.", false);
		//optional
		addFloat("max_af", "Maximum overall allele frequency in public databases. '0.01' means 1% allele frequency!", true, -1.0);
		addFloat("max_af_sub", "Maximum sub-population allele frequency in public databases. '0.01' means 1% allele frequency!", true, -1.0);
		addString("impact", "Comma-separated list of SnpEff impacts that pass.", "");
		addInt("max_ihdb", "Maximum in-house database count.", true, -1);
		addFlag("max_ihdb_ignore_genotype", "If set, variant genotype is ignored. Otherwise, only homozygous database entries are counted for homozygous variants, and all entries are count for heterozygous variants.");
		addInt("min_class", "Minimum classification of *classified* variants.", true, -1);
		addString("filters", "Comma-separated list of filter column entries to remove.", true, "");
		addFlag("comphet", "If set, only hompound-heterozygous variants pass. Performed after all other filters!");
		addString("genotype", "If set, only variants with the specified genotype pass. Performed after all other filters!", true, "");

		changeLog(2017, 6, 14, "Added sub-population allele frequency filter.");
		changeLog(2016, 6, 11, "Initial commit.");
	}

	virtual void main()
	{
		//load variants
		VariantList variants;
		variants.load(getInfile("in"));

		VariantFilter filter(variants);

		//filter AF
		double max_af = getFloat("max_af");
		if (max_af>=0)
		{
			filter.flagByAlleleFrequency(max_af);
		}

		//filter AF sub-populations
		double max_af_sub = getFloat("max_af_sub");
		if (max_af_sub>=0)
		{
			filter.flagBySubPopulationAlleleFrequency(max_af_sub);
		}

		//filter impact
		QString impact = getString("impact");
		if (!impact.isEmpty())
		{
			filter.flagByImpact(impact.split(","));
		}

		//filter IHDB
		int max_ihdb = getInt("max_ihdb");
		if (max_ihdb>0)
		{
			QStringList geno_cols;
			if (!getFlag("max_ihdb_ignore_genotype"))
			{
				geno_cols << "genotype";
			}
			filter.flagByIHDB(max_ihdb, geno_cols);
		}

		//filter classification
		int min_class = getInt("min_class");
		if (min_class!=-1)
		{
			filter.flagByClassification(min_class);
		}

		//filter filter column
		QString filters = getString("filters");
		if (!filters.isEmpty())
		{
			filter.flagByFilterColumnMatching(filters.split(","));
		}

		//filter genotype
		QString genotype = getString("genotype");
		if (!genotype.isEmpty())
		{
			filter.flagByGenotype(genotype);
		}

		//filter compound-heterozygous
		if (getFlag("comphet"))
		{
			filter.flagCompoundHeterozygous(QStringList() << "genotype");
		}

		//store variants
		filter.removeFlagged();
		variants.store(getOutfile("out"));
	}
};

#include "main.moc"

int main(int argc, char *argv[])
{
	ConcreteTool tool(argc, argv);
	return tool.execute();
}
