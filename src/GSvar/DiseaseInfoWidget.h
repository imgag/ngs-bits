#ifndef DISEASEINFODIALOG_H
#define DISEASEINFODIALOG_H

#include <QDialog>
#include "ui_DiseaseInfoWidget.h"
#include "NGSD.h"

class DiseaseInfoWidget
	: public QWidget
{
	Q_OBJECT

public:
	DiseaseInfoWidget(QString sample_id, QWidget *parent = 0);

	QString diseaseGroup() const;
	QString diseaseStatus() const;

	bool diseaseInformationMissing() const;

private:
	Ui::DiseaseInfoWidget ui_;
};

#endif // DISEASEINFODIALOG_H
