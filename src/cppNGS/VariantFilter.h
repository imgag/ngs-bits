#ifndef VARIANTFILTER_H
#define VARIANTFILTER_H

#include <QBitArray>
#include "BedFile.h"
#include "cppNGS_global.h"
class VariantList;

///Variant filtering engine.
///Variants are first flagged by several subsequent filters and then filtered according to the flags.
class CPPNGSSHARED_EXPORT VariantFilter
{
	public:
		///Constructor.
		VariantFilter(VariantList& vl);

		///Flags variants by allele frequency in public databases (1000g, ExAC, etc.).
		void flagByAllelFrequency(double max_af);

		///Flags variants by SnpEff impact.
		void flagByImpact(QStringList impacts);

		///Flags variants by in-house database count (from NGSD). If genotype is not ignored, for homozygous variants only homozygous NGSD variants are counted.
		void flagByIHDB(int max_count, bool ignore_genotype);

		///Flags variants by filter column entries.
		void flagByFilterColumn(QStringList remove);

		///Flags variants by classification filter.
		void flagByClassification(int min_class);

		///Flags variants by genes filter.
		void flagByGenes(QStringList genes);


		///Flags variants by annotations filter.
		void flagByGenotype(QString genotype);

		///Flags *heterozygous* variants (only compound-heterozygous variants pass).
		void flagCompoundHeterozygous();


		///Flags variants by region filter.
		void flagByRegions(const BedFile& regions);

		///Flags variants by region filter.
		void flagByRegion(const BedLine& region);

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

		///Applies the flags. Variants with 'true' flag are kept, all other variants are removed.
		void apply();

	protected:
		VariantList& variants;
		QBitArray pass;
};

#endif // VARIANTFILTER_H
