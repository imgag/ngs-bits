#ifndef DISEASEINFODIALOG_H
#define DISEASEINFODIALOG_H

#include <QDialog>
#include "ui_DiseaseInfoDialog.h"
#include "NGSD.h"

class DiseaseInfoDialog
	: public QDialog
{
	Q_OBJECT

public:
	DiseaseInfoDialog(QString sample_id, QWidget *parent = 0);

	bool sampleNameIsValid() const;
	bool diseaseInformationMissing() const;

private slots:
	void updateSampleDatabaseEntry();

private:
	Ui::DiseaseInfoDialog ui_;
	NGSD db_;
	QString sample_id_;
};

#endif // DISEASEINFODIALOG_H
