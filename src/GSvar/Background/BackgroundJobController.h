#ifndef BACKGROUNDJOBCONTROLLER_H
#define BACKGROUNDJOBCONTROLLER_H

#include <QObject>
#include <QThreadPool>
#include <QDateTime>

#include "BackgroundWorkerBase.h"
#include "BusyDialog.h"

struct JobInfo
{
	int id = -1;
	QString name;
	QDateTime started;
	QString status;
	int elapsed_ms = -1;
	QString messages;

	BusyDialog* busy_dlg = nullptr;
};

class BackgroundJobController
	: public QObject
{
	Q_OBJECT

public:
	static BackgroundJobController& instance();

	int start(BackgroundWorkerBase* job, bool show_busy_dialog);
	QString getJobStatus(int job_id);
	QString getJobMessages(int job_id);
	const QList<JobInfo>& getJobs();

public slots:
	void started();
	void finished();
	void failed();

signals:
	void jobChanged(int job_id);
	void showBusyDialog(QString job_name, BusyDialog* jobinfo_dlg);

protected:
	BackgroundJobController();

private:
	QThreadPool pool_;
	int next_id_;
	QList<JobInfo> jobs_;

};

#endif // BACKGROUNDJOBCONTROLLER_H
