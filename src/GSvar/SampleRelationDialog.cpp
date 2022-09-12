#include "SampleRelationDialog.h"

SampleRelationDialog::SampleRelationDialog(QWidget* parent)
	: QDialog(parent)
	, ui_()
{
	ui_.setupUi(this);
	connect(ui_.sample1, SIGNAL(textChanged(QString)), this, SLOT(updateOkButton()));
	connect(ui_.sample2, SIGNAL(textChanged(QString)), this, SLOT(updateOkButton()));
	connect(ui_.relation, SIGNAL(currentTextChanged(QString)), this, SLOT(updateOkButton()));
	connect(ui_.swap_btn, SIGNAL(clicked(bool)), this, SLOT(swapSamples()));

	//fill samples
	NGSD db;
	DBTable sample_table = db.createTable("sample", "SELECT id, name FROM sample");
	ui_.sample1->fill(sample_table);
	ui_.sample2->fill(sample_table);

	//fill relations
	QStringList relations;
	relations << "" << db.getEnum("sample_relations", "relation");
	ui_.relation->addItems(relations);
}

void SampleRelationDialog::setSample1(QString sample_name, bool enabled)
{
	ui_.sample1->setText(sample_name);
	ui_.sample1->setEnabled(enabled);
}

void SampleRelationDialog::setSample2(QString sample_name, bool enabled)
{
	ui_.sample2->setText(sample_name);
	ui_.sample2->setEnabled(enabled);
}

SampleRelation SampleRelationDialog::sampleRelation() const
{
	return SampleRelation{ui_.sample1->text().toUtf8(), ui_.relation->currentText().toUtf8(), ui_.sample2->text().toUtf8()};
}

void SampleRelationDialog::swapSamples()
{
	//get data
	QString s1t = ui_.sample1->text();
	QString s2t = ui_.sample2->text();
	bool s1e = ui_.sample1->isEnabled();
	bool s2e = ui_.sample2->isEnabled();

	//set data
	ui_.sample1->setText(s2t);
	ui_.sample2->setText(s1t);
	ui_.sample1->setEnabled(s2e);
	ui_.sample2->setEnabled(s1e);
}

void SampleRelationDialog::updateOkButton()
{
	bool enabled = ui_.sample1->isValidSelection() && !ui_.relation->currentText().isEmpty() && ui_.sample2->isValidSelection();
	ui_.ok_btn->setEnabled(enabled);
}
