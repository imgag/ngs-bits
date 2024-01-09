#ifndef NGSDCACHEINITIALIZER_H
#define NGSDCACHEINITIALIZER_H

#include "BackgroundWorkerBase.h"

//Initializes NGSD gene/transcript cache
class NGSDCacheInitializer
	: public BackgroundWorkerBase
{
    Q_OBJECT

public:
	NGSDCacheInitializer();
	void process() override;
};

#endif // NGSDCACHEINITIALIZER_H
