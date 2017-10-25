#ifndef PHENOTOGENESDIALOG_H
#define PHENOTOGENESDIALOG_H

#include <QDialog>
#include "ui_PhenoToGenesDialog.h"

class PhenoToGenesDialog
	: public QDialog
{
	Q_OBJECT

public:
	PhenoToGenesDialog(QWidget* parent = 0);

private slots:
	void copyGenesToClipboard();
	void storeGenesAsTSV();

	void tabChanged(int);

private:
	Ui::PhenoToGenesDialog ui;
};

#endif // PHENOTOGENESDIALOG_H
