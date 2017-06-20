#include "Exceptions.h"
#include "ToolBase.h"
#include "ChromosomalIndex.h"
#include "VariantList.h"
#include "BedFile.h"
#include "Helper.h"
#include "VariantFilter.h"
#include "NGSHelper.h"

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
		addEnum("geno_affected", "If set, only variants with the specified genotype in affected samples pass. Performed after all other filters!", true, QStringList() << "hom" << "het" << "comphet" << "comphet+hom" << "any", "any");
		addEnum("geno_control", "If set, only variants with the specified genotype in control samples pass. Performed after all other filters!", true, QStringList() << "hom" << "het" << "wt" << "not_hom" << "any", "any");

		changeLog(2017, 6, 14, "Refactoring of genotype-based filters: now also supports multi-sample filtering of affected and control samples.");
		changeLog(2017, 6, 14, "Added sub-population allele frequency filter.");
		changeLog(2016, 6, 11, "Initial commit.");
	}

	virtual void main()
	{
		//load variants
		QString in = getInfile("in");
		VariantList variants;
		variants.load(in);

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
		QStringList samples_affected = NGSHelper::getSampleHeader(variants, in).sampleColumns(true);
		int max_ihdb = getInt("max_ihdb");
		if (max_ihdb>0)
		{
			filter.flagByIHDB(max_ihdb, getFlag("max_ihdb_ignore_genotype") ? QStringList() : samples_affected);
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

		//filter genotype (affected)
		QString geno_affected = getEnum("geno_affected");
		if (geno_affected=="comphet")
		{
			filter.flagCompoundHeterozygous(samples_affected);
		}
		else if (geno_affected=="comphet+hom")
		{
			filter.flagCompoundHeterozygous(samples_affected, true);
		}
		else if (geno_affected!="any")
		{
			filter.flagByGenotype(geno_affected, samples_affected);
		}

		//filter genotype (control)
		QString geno_control = getEnum("geno_control");
		if (geno_control!="any")
		{
			QStringList samples_control = NGSHelper::getSampleHeader(variants, in).sampleColumns(false);
			bool invert = false;
			if (geno_control.startsWith("not_"))
			{
				geno_control = geno_control.mid(4);
				invert = true;
			}
			filter.flagByGenotype(geno_control, samples_control, invert);
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
