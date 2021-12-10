#ifndef CACHEINITWORKER_H
#define CACHEINITWORKER_H

#include "WorkerBase.h"

class CacheInitWorker
	: public WorkerBase
{
public:
	CacheInitWorker();
	virtual void process() override;
};

#endif // CACHEINITWORKER_H
