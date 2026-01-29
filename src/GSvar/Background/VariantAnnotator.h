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

signals:
	//signal that is emitted when the annotation is done (in addition to the regular finished() signal)
	void loadFile(QString);

private:
	VariantList variants_;
};

#endif


