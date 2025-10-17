#ifndef DISEASEINFODIALOG_H
#define DISEASEINFODIALOG_H

#include "ui_DiseaseInfoWidget.h"

class DiseaseInfoWidget
	: public QWidget
{
	Q_OBJECT

public:
	DiseaseInfoWidget(QString ps_name, QString sample_id, QWidget *parent = 0);

	QString diseaseGroup() const;
	QString diseaseStatus() const;

	bool diseaseInformationMissing() const;

private:
	Ui::DiseaseInfoWidget ui_;
	QString ps_name_;
};

#endif // DISEASEINFODIALOG_H
