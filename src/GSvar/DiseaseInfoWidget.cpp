#include "DiseaseInfoWidget.h"
#include "NGSD.h"

DiseaseInfoWidget::DiseaseInfoWidget(QString ps_name, QString sample_id, QWidget* parent)
	: QWidget(parent)
	, ui_()
	, ps_name_(ps_name)
{
	NGSD db;

	//setup UI
	ui_.setupUi(this);
	ui_.group->insertItems(0, db.getEnum("sample", "disease_group"));
	ui_.status->insertItems(0, db.getEnum("sample", "disease_status"));

	//get sample data
	SampleData sample_data = db.getSampleData(sample_id);
	ui_.group->setCurrentText(sample_data.disease_group);
	ui_.status->setCurrentText(sample_data.disease_status);
}

bool DiseaseInfoWidget::diseaseInformationMissing() const
{
	return diseaseGroup()=="n/a" || diseaseStatus()=="n/a";
}

QString DiseaseInfoWidget::diseaseGroup() const
{
	return ui_.group->currentText();
}

QString DiseaseInfoWidget::diseaseStatus() const
{
	return ui_.status->currentText();
}
