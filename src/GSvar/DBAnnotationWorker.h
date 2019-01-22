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
	DBAnnotationWorker(QString ps_name, VariantList& variants, BusyDialog* busy, QString roi_file="", double max_af=0.0);
	virtual void process();
	bool targetRegionOnly();

private:
	//input variables
	QString ps_name_;
	VariantList& variants_;
	QString roi_file_;
	double max_af_;
	NGSD ngsd_;
};

#endif


