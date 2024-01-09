#ifndef VARIANTANNOTATOR_H
#define VARIANTANNOTATOR_H

#include "Background/BackgroundWorkerBase.h"
#include "GermlineReportGenerator.h"

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


