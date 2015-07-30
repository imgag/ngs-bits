#ifndef DBANNOTATIONWORKER_H
#define DBANNOTATIONWORKER_H

#include <QString>
#include "VariantList.h"
#include "WorkerBase.h"
#include "BusyDialog.h"
#include "NGSD.h"
#include "GPD.h"

///Database annotation worker (NGSD/GPD).
class DBAnnotationWorker
		: public WorkerBase
{
	Q_OBJECT

public:
	DBAnnotationWorker(QString filename, VariantList& variants, BusyDialog* busy);
	virtual void process();

private:
	//input variables
	QString filename_;
	VariantList& variants_;
	GPD gpd_;
	NGSD ngsd_;
};

#endif


