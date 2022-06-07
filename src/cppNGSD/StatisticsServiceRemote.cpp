#include "StatisticsServiceRemote.h"
#include "ApiCaller.h"

StatisticsServiceRemote::StatisticsServiceRemote()	
{

}

BedFile StatisticsServiceRemote::lowCoverage(const BedFile& bed_file, const QString& bam_file, int cutoff) const
{
	qDebug() << "bed_file.toText()" << bed_file.toText().toUtf8();

	ApiCaller().post("low_coverage_regions", RequestUrlParams(), HttpHeaders(), QString("roi="+bed_file.toText().toUtf8()+"&bam_file="+bam_file+"&cutoff="+QString::number(cutoff)).toLocal8Bit(), true);

	return BedFile();
}
