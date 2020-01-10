#ifndef GSVARSTOREWORKER_H
#define GSVARSTOREWORKER_H

#include "WorkerBase.h"
#include "VariantList.h"

///Report generation worker.
class GSvarStoreWorker
		: public WorkerBase
{
	Q_OBJECT

public:
	GSvarStoreWorker(const VariantList& variants, QString filename);
	~GSvarStoreWorker();

	virtual void process();

private:
	const VariantList& variants_;
	QString filename_;
};

#endif //GSVARSTOREWORKER_H


