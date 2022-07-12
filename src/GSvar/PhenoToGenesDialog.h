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
	void copyGenesToClipboardAsTable();
	void copyGenesToClipboardAsList();
	void storeGenesAsTSV();
	void showSettings();
	void tabChanged(int);

private:
	Ui::PhenoToGenesDialog ui;
	PhenotypeSettings phenotype_settings_;
};

#endif // PHENOTOGENESDIALOG_H
