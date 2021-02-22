#ifndef REPORTWORKER_H
#define REPORTWORKER_H

#include "WorkerBase.h"
#include "GermlineReportGenerator.h"

///Report generation worker.
class ReportWorker
		: public WorkerBase
{
	Q_OBJECT

public:
	///Constructor.
	ReportWorker(GermlineReportGeneratorData data, QString filename);
	virtual void process();

	///Returns the file to which the report was written.
	QString getReportFile()
	{
		return filename_;
	}

	///Move temporary report file to actual output location
	static void moveReport(QString temp_filename, QString filename);

private:
	GermlineReportGeneratorData data_;
	QString filename_;
};

#endif


