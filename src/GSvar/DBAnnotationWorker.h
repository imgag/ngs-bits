#ifndef DBANNOTATIONWORKER_H
#define DBANNOTATIONWORKER_H

#include <QString>
#include "VariantList.h"
#include "WorkerBase.h"

///Database annotation worker (NGSD/GPD).
class DBAnnotationWorker
		: public WorkerBase
{
	Q_OBJECT

public:
	DBAnnotationWorker(QString filename, QString genome, VariantList& variants);
	virtual void process();

private:
	//input variables
	QString filename_;
	QString genome_;
	VariantList& variants_;
};

#endif


