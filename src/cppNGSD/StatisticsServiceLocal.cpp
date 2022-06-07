#include "StatisticsServiceLocal.h"
#include "Statistics.h"

StatisticsServiceLocal::StatisticsServiceLocal()
{

}

BedFile StatisticsServiceLocal::lowCoverage(const BedFile& bed_file, const QString& bam_file, int cutoff) const
{	
	return Statistics::lowCoverage(bed_file, bam_file, cutoff);
}
