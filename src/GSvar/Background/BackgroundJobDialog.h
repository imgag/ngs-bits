#ifndef BACKGROUNDJOBDIALOG_H
#define BACKGROUNDJOBDIALOG_H

#include "ui_BackgroundJobDialog.h"
#include "BackgroundJobController.h"

class BackgroundJobDialog
	: public QDialog
{
	Q_OBJECT

public:
	 BackgroundJobDialog(QWidget* parent);

private slots:
	void showBusyDialog(QString name, BusyDialog* jobinfo_dlg);
	void updateTable(int id=-1);

private:
	Ui::BackgroundJobDialog ui_;
	BackgroundJobController& job_controller_;

	//Re-draws the table (or only one line, if id is given)

};

#endif // BACKGROUNDJOBDIALOG_H
