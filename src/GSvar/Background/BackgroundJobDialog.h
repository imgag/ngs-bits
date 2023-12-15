#ifndef BACKGROUNDJOBDIALOG_H
#define BACKGROUNDJOBDIALOG_H

#include "ui_BackgroundJobDialog.h"
#include "Background/BackgroundWorkerBase.h"
#include <QThreadPool>


class BackgroundJobDialog
	: public QDialog
{
	Q_OBJECT

public:
	 BackgroundJobDialog(QWidget *parent = nullptr);
	 void start(BackgroundWorkerBase* job);

private slots:
	 void started(int id);
	 void finished(int id, int elapsed_ms);
	 void failed(int id, int elapsed_ms, QString error);
	 void updateTableWidths();

private:
	Ui::BackgroundJobDialog ui_;
	QThreadPool pool_;
	int next_id_;
};

#endif // BACKGROUNDJOBDIALOG_H
