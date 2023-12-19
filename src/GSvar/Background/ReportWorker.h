#ifndef REPORTWORKER_H
#define REPORTWORKER_H

#include "Background/BackgroundWorkerBase.h"
#include "GermlineReportGenerator.h"

///Report generation worker.
class ReportWorker
		: public BackgroundWorkerBase
{
	Q_OBJECT

public:
	ReportWorker(GermlineReportGeneratorData data, QString filename);
	void process() override;
	void userInteration() override;

private:
	GermlineReportGeneratorData data_;
	QString filename_;
};

#endif


