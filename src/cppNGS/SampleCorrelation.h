#ifndef SAMPLECORRELATION_H
#define SAMPLECORRELATION_H

#include "cppNGS_global.h"
#include <QStringList>

class CPPNGSSHARED_EXPORT SampleCorrelation
{
public:
	int no_variants1;
	int no_variants2;
	int total_variants;
	double correlation;
	double ol_perc;			//percentage overlapping snps
	QStringList messages;

	void CalculateFromVcf(QString& in1, QString& in2, int window);
	void CalculateFromBam(QString& in1, QString& in2, int min_cov, int max_snps);

private:
	double genoToDouble(const QString& geno);
};

#endif // SAMPLECORRELATION_H
