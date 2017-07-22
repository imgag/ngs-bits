#ifndef SAMPLECORRELATION_H
#define SAMPLECORRELATION_H

#include "cppNGS_global.h"
#include <QStringList>

class CPPNGSSHARED_EXPORT SampleCorrelation
{
public:

	int noVariants1()
	{
		return no_variants1_;
	}

	int noVariants2()
	{
		return no_variants2_;
	}

	int totalVariants()
	{
		return total_variants_;
	}

	double sampleCorrelation()
	{
		return sample_correlation_;
	}

	double olPerc()
	{
		return ol_perc_;
	}

	QStringList& messages()
	{
		return messages_;
	}

	void calculateFromVcf(QString& in1, QString& in2, int window);
	void calculateFromBam(QString& in1, QString& in2, int min_cov, int max_snps, QString roi_file = "");

private:
	double genoToDouble(const QString& geno);
	int no_variants1_;
	int no_variants2_;
	int total_variants_;
	double sample_correlation_;
	double ol_perc_;			//percentage overlapping snps
	QStringList messages_;
};

#endif // SAMPLECORRELATION_H
