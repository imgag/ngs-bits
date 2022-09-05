#include "StatisticsServiceRemote.h"
#include "ApiCaller.h"

StatisticsServiceRemote::StatisticsServiceRemote()	
{

}

BedFile StatisticsServiceRemote::lowCoverage(const BedFile& bed_file, const QString& bam_file, int cutoff) const
{	
	BedFile output;
	QStringList bam_file_parts = bam_file.split("/");
	if (bam_file_parts.count()<=1) THROW(ArgumentException, "BAM file URL id is missing");
	output = output.fromText(ApiCaller().post("low_coverage_regions", RequestUrlParams(), HttpHeaders(), QString("roi="+bed_file.toText().toUtf8()+"&bam_url_id="+bam_file_parts[bam_file_parts.count()-2]+"&cutoff="+QString::number(cutoff)).toUtf8(), true));

	return output;
}

void StatisticsServiceRemote::avgCoverage(BedFile& bed_file, const QString& bam_file) const
{
	QStringList bam_file_parts = bam_file.split("/");
	if (bam_file_parts.count()<=1) THROW(ArgumentException, "BAM file URL id is missing");
	bed_file = bed_file.fromText(ApiCaller().post("avg_coverage_gaps", RequestUrlParams(), HttpHeaders(), QString("low_cov="+bed_file.toText().toUtf8()+"&bam_url_id="+bam_file_parts[bam_file_parts.count()-2]).toUtf8(), true));
}

double StatisticsServiceRemote::targetRegionReadDepth(const BedFile& bed_file, const QString& bam_file) const
{
	QStringList bam_file_parts = bam_file.split("/");
	if (bam_file_parts.count()<=1) THROW(ArgumentException, "BAM file URL id is missing");
	QByteArray response = ApiCaller().post("target_region_read_depth", RequestUrlParams(), HttpHeaders(), QString("regions="+bed_file.toText().toUtf8()+"&bam_url_id="+bam_file_parts[bam_file_parts.count()-2]).toUtf8(), true);

	return response.toDouble();
}
