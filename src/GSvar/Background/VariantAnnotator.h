#ifndef VARIANTANNOTATOR_H
#define VARIANTANNOTATOR_H

#include "VariantList.h"
#include "Background/BackgroundWorkerBase.h"

///Report generation worker.
class VariantAnnotator
		: public BackgroundWorkerBase
{
	Q_OBJECT

public:
	VariantAnnotator(const VariantList& variants);
	void process() override;

private:
	VariantList variants_;
};

#endif


