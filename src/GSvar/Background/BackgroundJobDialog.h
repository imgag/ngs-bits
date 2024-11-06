#ifndef BACKGROUNDJOBDIALOG_H
#define BACKGROUNDJOBDIALOG_H

#include "ui_BackgroundJobDialog.h"
#include "Background/BackgroundWorkerBase.h"
#include "BusyDialog.h"
#include <QThreadPool>
#include <QDateTime>

class BackgroundJobDialog
	: public QDialog
{
	Q_OBJECT

public:
	 BackgroundJobDialog(QWidget* parent);
     int start(BackgroundWorkerBase* job, bool show_busy_dialog);
     QString getJobStatus(int job_id);
     QString getJobMessages(int job_id);

private slots:
	 void started();
	 void finished();
	 void failed();

private:
	Ui::BackgroundJobDialog ui_;
	QThreadPool pool_;
	int next_id_;
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
	QList<JobInfo> jobs_;

	//Re-draws the table (or only one line, if id is given)
	void updateTable(int id=-1);
};

#endif // BACKGROUNDJOBDIALOG_H
