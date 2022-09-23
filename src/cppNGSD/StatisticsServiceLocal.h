#ifndef STATISTICSSERVICELOCAL_H
#define STATISTICSSERVICELOCAL_H

#include "cppNGSD_global.h"
#include "StatisticsService.h"
#include "Settings.h"
#include "Exceptions.h"

class CPPNGSDSHARED_EXPORT StatisticsServiceLocal
	: virtual public StatisticsService
{
public:
	StatisticsServiceLocal();
	virtual ~StatisticsServiceLocal() {}	
	virtual BedFile lowCoverage(BedFile& bed_file, const QString& bam_file, int cutoff) const override;
	virtual void avgCoverage(BedFile& bed_file, const QString& bam_file, int threads) const override;
	virtual double targetRegionReadDepth(const BedFile& bed_file, const QString& bam_file, int threads) const override;
};

#endif // STATISTICSSERVICELOCAL_H
