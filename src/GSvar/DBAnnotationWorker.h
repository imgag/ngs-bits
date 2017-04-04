#ifndef DBANNOTATIONWORKER_H
#define DBANNOTATIONWORKER_H

#include <QString>
#include "VariantList.h"
#include "WorkerBase.h"
#include "BusyDialog.h"
#include "NGSD.h"

///Database annotation worker for NGSD.
class DBAnnotationWorker
		: public WorkerBase
{
	Q_OBJECT

public:
	DBAnnotationWorker(QString filename, VariantList& variants, BusyDialog* busy, QString roi_file="");
	virtual void process();
	bool targetRegionOnly();

private:
	//input variables
	QString filename_;
	VariantList& variants_;
	QString roi_file_;
	NGSD ngsd_;
};

#endif


