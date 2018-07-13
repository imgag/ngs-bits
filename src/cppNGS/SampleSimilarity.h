#ifndef SAMPLESIMILARITY_H
#define SAMPLESIMILARITY_H

#include "cppNGS_global.h"
#include <QStringList>

// Sample similarity calculator
class CPPNGSSHARED_EXPORT SampleSimilarity
{
public:

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

	// Percentage of overlapping variants - only for VCF-based calculation
	double olPerc()
	{
		return ol_perc_;
	}

	// Correlation
	double sampleCorrelation()
	{
		return sample_correlation_;
	}

	//Percentage of variants with IDS zero (e.g. AA and GG) - only for BAM-based calculation
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

	//Calculation from VCF
	void calculateFromVcf(QString& in1, QString& in2, int window, bool include_gonosomes);

	//Calculation from BAM
	void calculateFromBam(QString build, QString& in1, QString& in2, int min_cov, int max_snps, bool include_gonosomes, QString roi_file = "");

	//Reset
	void clear();

private:
	double genoToDouble(const QString& geno);
	int no_variants1_;
	int no_variants2_;
	int total_variants_;
	double sample_correlation_;
	double ol_perc_;
	double ibs0_perc_;
	double ibs2_perc_;
	QStringList messages_;
};

#endif // SAMPLESIMILARITY_H
