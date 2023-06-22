#ifndef ANALYSISWORKER_H
#define ANALYSISWORKER_H

#include <QRunnable>
#include <QObject>
#include "Auxilary.h"
#include "NGSD.h"

//Export worker
class ExportWorker
	: public QObject
	, public QRunnable
{
	Q_OBJECT

public:
	ExportWorker(QString chr, const ExportParameters& params, const SharedData& shared_data);
	virtual ~ExportWorker();
	virtual void run() override;

signals:
	void log(QString chr, QString message); //signal emitted when something needs to be written to the log
	void done(QString chr); //signal emitted when job was successful
	void error(QString chr, QString message); //signal emitted when job failed

private slots:
	void storeCountCache(NGSD& db, QHash<int, GenotypeCounts>& count_cache);

private:
	QString chr_;
	const ExportParameters& params_;
	const SharedData& shared_data_;
};

#endif


