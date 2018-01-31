#include "DiseaseInfoDialog.h"

DiseaseInfoDialog::DiseaseInfoDialog(QString sample_id, QWidget *parent)
	: QDialog(parent)
	, ui_()
	, sample_id_(sample_id)
{
	//setup UI
	ui_.setupUi(this);
	ui_.group->insertItems(0, db_.getEnum("sample", "disease_group"));
	ui_.status->insertItems(0, db_.getEnum("sample", "disease_status"));
	connect(this, SIGNAL(accepted()), this, SLOT(updateSampleDatabaseEntry()));

	//get sample data
	SampleData sample_data = db_.getSampleData(sample_id_);
	ui_.group->setCurrentText(sample_data.disease_group);
	ui_.status->setCurrentText(sample_data.disease_status);
}

bool DiseaseInfoDialog::diseaseInformationMissing() const
{
	return ui_.group->currentText()=="n/a" || ui_.status->currentText()=="n/a";
}

void DiseaseInfoDialog::updateSampleDatabaseEntry()
{
	db_.setSampleDiseaseData(sample_id_, ui_.group->currentText(), ui_.status->currentText());
}
