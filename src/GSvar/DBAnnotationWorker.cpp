#include "DBAnnotationWorker.h"
#include "Exceptions.h"
#include "Log.h"

DBAnnotationWorker::DBAnnotationWorker(QString filename, VariantList& variants, BusyDialog* busy, QString roi_file)
	: WorkerBase("Database annotation")
	, filename_(filename)
	, variants_(variants)
	, roi_file_(roi_file)
	, ngsd_()
{
	connect(&ngsd_, SIGNAL(initProgress(QString, bool)), busy, SLOT(init(QString, bool)));
	connect(&ngsd_, SIGNAL(updateProgress(int)), busy, SLOT(update(int)));
}

void DBAnnotationWorker::process()
{
	try
	{
		BedFile roi;
		if (roi_file_!="")
		{
			roi.load(roi_file_);
		}
		ngsd_.annotate(variants_, filename_, roi);
	}
	catch (Exception& e)
	{
		error_message_ = e.message();
	}
	catch(std::exception& e)
	{
		error_message_ = e.what();
	}
	catch(...)
	{
		error_message_ = "Unknown exception!";
	}
}

bool DBAnnotationWorker::targetRegionOnly()
{
	return roi_file_!="";
}

