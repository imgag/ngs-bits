#ifndef STATISTICSWORKER_H
#define STATISTICSWORKER_H

#include "BedFile.h"
#include "QCCollection.h"
#include "WorkerBase.h"
#include <QObject>
#include <QMap>

///Mapping statistics worker for background-calculation of mapping statistics.
class StatisticsWorker
		: public WorkerBase
{
	Q_OBJECT

public:
	StatisticsWorker(const BedFile& bed_file, const QString& bam_file);
	QCCollection result();
	virtual void process();

private:
	BedFile bed_file_;
	QString bam_file_;
	QCCollection result_;
};

#endif // STATISTICSWORKER_H


