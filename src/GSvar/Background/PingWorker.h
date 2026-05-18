#ifndef PINGWORKER_H
#define PINGWORKER_H

#include "BackgroundWorkerBase.h"

//Initializes NGSD gene/transcript cache
class PingWorker
	: public BackgroundWorkerBase
{
	Q_OBJECT

public:
	PingWorker();
	void process() override;
};

#endif // PINGWORKER_H
