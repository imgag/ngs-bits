#ifndef SAMPLESIMILARITY_H
#define SAMPLESIMILARITY_H

#include "cppNGS_global.h"
#include "BedFile.h"
#include "Statistics.h"
#include <QStringList>
#include <QHash>

// Sample similarity calculator
class CPPNGSSHARED_EXPORT SampleSimilarity
{
public:

	//Variant representation.
	typedef QHash<const QChar*, float> VariantGenotypes;

	//Extract genotypes from VCF (no WT genotype).
	static VariantGenotypes genotypesFromVcf(QString filename, bool include_gonosomes, bool skip_multi, const BedFile& roi);
	static VariantGenotypes genotypesFromVcf(QString filename, bool include_gonosomes, bool skip_multi);

	//Extract genotypes from GSvar (no WT genotype, includes only HIGH/MODERATE/LOW impact variants because only rare or known pathogenic intronic/intergenic are stored in GSvar files for WGS).
	static VariantGenotypes genotypesFromGSvar(QString filename, bool include_gonosomes, const BedFile& roi);
	static VariantGenotypes genotypesFromGSvar(QString filename, bool include_gonosomes);

	//Extract genotypes from BAM
	static VariantGenotypes genotypesFromBam(GenomeBuild build, const QString& filename, int min_cov, int max_snps, bool include_gonosomes, const BedFile& roi, const QString& ref_file = QString(), bool include_single_end_reads=false);
	static VariantGenotypes genotypesFromBam(GenomeBuild build, const QString& filename, int min_cov, int max_snps, bool include_gonosomes, const QString& ref_file = QString(), bool include_single_end_reads=false);

	//Calculation of similarity
	void calculateSimilarity(const VariantGenotypes& in1, const VariantGenotypes& in2);

	// Number of variants in first sample
	int noVariants1()
	{
		return no_variants1_;
	}

	// Number of variants in second sample
	int noVariants2()
	{
		return no_variants2_;
	}

	// Percentage of overlapping variants
	double olPerc()
	{
		return ol_perc_;
	}

	// Number of overlapping variants
	double olCount()
	{
		return ol_count_;
	}

	// Correlation
	double sampleCorrelation()
	{
		return sample_correlation_;
	}

	//Percentage of variants with IDS zero (e.g. AA and GG) - only meaningful for BAM-based calculation
	double ibs0Perc()
	{
		return ibs0_perc_;
	}

	//Percentage of variants with IDS two (e.g. AA and AA)
	double ibs2Perc()
	{
		return ibs2_perc_;
	}

	//Output messages
	QStringList& messages()
	{
		return messages_;
	}

	//Reset
	void clear();

private:
	static float genoToDouble(const QString& geno);

	static VariantGenotypes genotypesVcf(const VcfFile& variants, const QString& filename, bool include_gonosomes, bool skip_multi);
	static VariantGenotypes genotypesGSvar(VariantList variants, QString filename, bool include_gonosomes);
	static VariantGenotypes genotypesBam(const VcfFile& snps, BamReader& reader, int min_cov, int max_snps, bool include_gonosomes, bool include_single_end_reads=false);

	//Returns a string pointer, which can be stored/compared instead of the string. Reduces memory and run-time.
	//Beanchmark with 38 GSvar files with 66k variants: memory-consumption 430>118MB, comparison time 20>5s
	static const QChar* strToPointer(const QString& str);
	int no_variants1_;
	int no_variants2_;
	double sample_correlation_;
	double ol_perc_;
	int ol_count_;
	double ibs0_perc_;
	double ibs2_perc_;
	QStringList messages_;
};

#endif // SAMPLESIMILARITY_H
