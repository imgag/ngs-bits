#include "StatisticsServiceLocal.h"
#include "Statistics.h"

StatisticsServiceLocal::StatisticsServiceLocal()
{

}

BedFile StatisticsServiceLocal::lowCoverage(const BedFile& bed_file, const QString& bam_file, int cutoff) const
{	
	QString ref_file = Settings::string("reference_genome");
	return Statistics::lowCoverage(bed_file, bam_file, cutoff, 1, 0, ref_file);
}

void StatisticsServiceLocal::avgCoverage(BedFile& bed_file, const QString& bam_file) const
{
	QString ref_file = Settings::string("reference_genome");
	Statistics::avgCoverage(bed_file, bam_file, 1, false, true, 2, ref_file);
}

double StatisticsServiceLocal::targetRegionReadDepth(const BedFile& bed_file, const QString& bam_file) const
{
	//caclulate depth for each part of the target region
	BedFile bed_file_anno = bed_file;
	bed_file_anno.clearAnnotations();
	Statistics::avgCoverage(bed_file_anno, bam_file, 1, false, true, 10);

	//calcualte overall depth
	double depth = 0.0;
	double length_sum = 0.0;
	for (int i=0; i<bed_file_anno.count(); ++i)
	{
		depth += bed_file_anno[i].annotations()[0].toDouble() * bed_file_anno[i].length();
		length_sum += bed_file_anno[i].length();
	}

	return depth/length_sum;
}
