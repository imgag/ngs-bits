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
	void setAllowedEvidences(QList<PhenotypeEvidence::Evidence> allowedEvidence);
	void setAllowedSources(QList<PhenotypeSource::Source> allowedSources);

private slots:
	void copyGenesToClipboardAsTable();
	void copyGenesToClipboardAsList();
	void storeGenesAsTSV();

	void tabChanged(int);

private:
	Ui::PhenoToGenesDialog ui;
};

#endif // PHENOTOGENESDIALOG_H
