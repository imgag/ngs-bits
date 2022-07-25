#include "StatisticsServiceLocal.h"
#include "Statistics.h"

StatisticsServiceLocal::StatisticsServiceLocal()
{

}

BedFile StatisticsServiceLocal::lowCoverage(const BedFile& bed_file, const QString& bam_file, int cutoff) const
{	
	return Statistics::lowCoverage(bed_file, bam_file, cutoff);
}

void StatisticsServiceLocal::avgCoverage(BedFile& bed_file, const QString& bam_file) const
{
	Statistics::lowCoverage(bed_file, bam_file, 1, false, true);
}

double StatisticsServiceLocal::targetRegionReadDepth(const BedFile& bed_file, const QString& bam_file) const
{
	double depth;
	QString ref_file = Settings::string("reference_genome");
	QCCollection stats = Statistics::mapping(bed_file, bam_file, ref_file);

	for (int i=0; i<stats.count(); ++i)
	{
		if (stats[i].accession()=="QC:2000025") depth = stats[i].asDouble();
	}
	return depth;
}
