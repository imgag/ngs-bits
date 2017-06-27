#include "DiseaseInfoDialog.h"

DiseaseInfoDialog::DiseaseInfoDialog(QString ps_name, QWidget *parent)
	: QDialog(parent)
	, ui_()
	, ps_name_(ps_name)
{
	//setup UI
	ui_.setupUi(this);
	ui_.group->insertItems(0, db_.getEnum("sample", "disease_group"));
	ui_.status->insertItems(0, db_.getEnum("sample", "disease_status"));
	connect(this, SIGNAL(accepted()), this, SLOT(updateSampleDatabaseEntry()));

	//get sample data
	if (db_.sampleId(ps_name, false)!="")
	{
		ui_.group->setCurrentText(db_.sampleDiseaseGroup(ps_name));
		ui_.status->setCurrentText(db_.sampleDiseaseStatus(ps_name));
	}
	else
	{
		ps_name_ = "";
	}
}

bool DiseaseInfoDialog::sampleNameIsValid() const
{
	return (ps_name_!="");
}

bool DiseaseInfoDialog::diseaseInformationMissing() const
{
	return ui_.group->currentText()=="n/a" || ui_.status->currentText()=="n/a";
}

void DiseaseInfoDialog::updateSampleDatabaseEntry()
{
	db_.setSampleDiseaseGroup(ps_name_, ui_.group->currentText());
	db_.setSampleDiseaseStatus(ps_name_, ui_.status->currentText());
}
