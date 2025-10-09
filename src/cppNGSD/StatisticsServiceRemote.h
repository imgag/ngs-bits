#ifndef STATISTICSSERVICEREMOTE_H
#define STATISTICSSERVICEREMOTE_H

#include "cppNGSD_global.h"
#include "StatisticsService.h"

class CPPNGSDSHARED_EXPORT StatisticsServiceRemote
	: virtual public StatisticsService
{
public:
	StatisticsServiceRemote();
	virtual ~StatisticsServiceRemote() {}	
	virtual BedFile lowCoverage(const BedFile& bed_file, const QString& bam_file, int cutoff) const override;
	virtual void avgCoverage(BedFile& bed_file, const QString& bam_file, int threads) const override;
	virtual double targetRegionReadDepth(const BedFile& bed_file, const QString& bam_file, int threads) const override;
};

#endif // STATISTICSSERVICEREMOTE_H
