#ifndef VARIANTFILTER_H
#define VARIANTFILTER_H

#include <QBitArray>
#include "BedFile.h"
#include "cppNGS_global.h"
class VariantList;
class GeneSet;

///Variant filtering engine.
///Variants are first flagged by several subsequent filters and then filtered according to the flags.
class CPPNGSSHARED_EXPORT VariantFilter
{
	public:
		///Constructor.
		VariantFilter(VariantList& vl);

		///Flags variants by allele frequency in public databases (1000g, ExAC, gnomAD).
		void flagByAlleleFrequency(double max_af);

		///Flags variants by sub-population allele frequency in public databases (ExAC).
		void flagBySubPopulationAlleleFrequency(double max_af);

		///Flags variants by SnpEff impact.
		void flagByImpact(QStringList impacts);

		///Flags variants by in-house database count (from NGSD). If genotype columns are given, for homozygous/heterozygous variants are handled differently.
		void flagByIHDB(int max_count, QStringList genotype_columns);

		///Flags variants by filter column entries (remove non-empty filter entries).
		void flagByFilterColumn();

		///Flags variants by filter column entries (remove matching entries).
		void flagByFilterColumnMatching(QStringList remove);

		///Flags variants by classification filter.
		void flagByClassification(int min_class);

		///Flags variants by genes filter.
		void flagByGenes(const GeneSet& genes);

		///Flags variants by annotations filter.
		void flagByGenotype(QString genotype, QStringList genotype_columns, bool invert = false);

		///Flags variants by letting only compound-heterozygous variants pass.
		void flagCompoundHeterozygous(QStringList genotype_columns, bool hom_also_passes = false);

		///Flags variants by region filter.
		void flagByRegions(const BedFile& regions);

		///Flags variants by region filter.
		void flagByRegion(const BedLine& region);

		///Flags variants by region filter.
		void flagByGenePLI(double min_pli);

		///Generic filtering.
		void flagGeneric(QString criteria);

		///Direct access to flags array (for custom filters).
		QBitArray& flags()
		{
			return pass;
		}

		///Clears flags, i.e. all variants pass.
		void clear();

		///Inverts the flags.
		void invert()
		{
			pass = ~pass;
		}

		///Returns the number of passing variants.
		int countPassing() const
		{
			return pass.count(true);
		}

		///Remove variants that did not pass the filter (with 'false' flag).
		void removeFlagged();

		///Tag variants that did not pass the filter using the 'filter' column.
		void tagNonPassing(QByteArray tag, QByteArray description);

	protected:
		VariantList& variants;
		QBitArray pass;
};

#endif // VARIANTFILTER_H
