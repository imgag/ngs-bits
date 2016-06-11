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
		setDescription("Filter a variant list based on variant annotations.");
		addInfile("in", "Input variant list.", false);
		addOutfile("out", "Output variant list. If unset, writes to STDOUT.", false);
		//optional
		addFloat("max_af", "Maximum allele frequency in public databases. '0.01' means 1% allele frequency!", true, -1.0);
		addString("impact", "Comma-separated list of SnpEff impacts that pass.", "");
		addInt("max_ihdb", "Maximum in-house database frequency. For homozygous variants, only homozygous database entries count. For heterozygous variants all entries count.", true, -1);
		addInt("min_class", "Minimum classification of *classified* variants.", true, -1);
		addString("filters", "Comma-separated list of filter column entries to remove.", true, "");
		addFlag("comphet", "If set, only hompound-heterozygous variants pass of the *heterozygous* variants. Performed after all other filters!");
		addString("genotype", "If set, only variants with that genotype pass. Performed after all other filters!", true, "");

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
			filter.flagByAllelFrequency(max_af);
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
			filter.flagByIHDB(max_ihdb);
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
			filter.flagByFilterColumn(filters.split(","));
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
			filter.flagCompoundHeterozygous();
		}

		//store variants
		filter.apply();
		variants.store(getOutfile("out"));
	}
};

#include "main.moc"

int main(int argc, char *argv[])
{
	ConcreteTool tool(argc, argv);
	return tool.execute();
}
