#include "DBAnnotationWorker.h"
#include "GPD.h"
#include "NGSD.h"
#include "Exceptions.h"


DBAnnotationWorker::DBAnnotationWorker(QString filename, QString genome, VariantList& variants)
	: WorkerBase("Database annotation")
	, filename_(filename)
	, genome_(genome)
	, variants_(variants)
{
}

void DBAnnotationWorker::process()
{
	try
	{
		GPD().annotate(variants_);
		NGSD().annotate(variants_, filename_, genome_, true);
	}
	catch (Exception& e)
	{
		error_message_ = e.message();
	}
}

