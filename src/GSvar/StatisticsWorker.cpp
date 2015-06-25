#include "StatisticsWorker.h"
#include "Statistics.h"
#include "Log.h"
#include "Exceptions.h"

StatisticsWorker::StatisticsWorker(const BedFile& bed_file, const QString& bam_file)
	: WorkerBase("Mapping statistics")
	, bed_file_(bed_file)
	, bam_file_(bam_file)
	, result_()
{
}

QCCollection StatisticsWorker::result()
{
	return result_;
}

void StatisticsWorker::process()
{
	result_ = Statistics::mapping(bed_file_, bam_file_);
}
