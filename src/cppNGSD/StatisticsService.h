#ifndef STATISTICSSERVICE_H
#define STATISTICSSERVICE_H

#include "cppNGSD_global.h"
#include "BedFile.h"

//Statistics access service interface.
class CPPNGSDSHARED_EXPORT StatisticsService
{
public:
	virtual BedFile lowCoverage(const BedFile& bed_file, const QString& bam_file, int cutoff) const = 0;
	virtual void avgCoverage(BedFile& bed_file, const QString& bam_file) const = 0;
	virtual double targetRegionReadDepth(const BedFile& bed_file, const QString& bam_file) const = 0;
};

#endif // STATISTICSSERVICE_H
