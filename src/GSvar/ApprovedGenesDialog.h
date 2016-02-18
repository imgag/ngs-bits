#ifndef APPROVEDGENESDIALOG_H
#define APPROVEDGENESDIALOG_H

#include "ui_ApprovedGenesDialog.h"
#include "NGSD.h"

class ApprovedGenesDialog
	: public QDialog
{
	Q_OBJECT

public:
	ApprovedGenesDialog(QWidget* parent = 0);

protected slots:
	void on_convertBtn_pressed();

private:
	Ui::ApprovedGenesDialog ui_;
	NGSD db_;
};

#endif // APPROVEDGENESDIALOG_H
