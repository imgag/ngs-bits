#include "ReportVariantDialog.h"
#include "Settings.h"
#include <QPushButton>

ReportVariantDialog::ReportVariantDialog(QString variant, QList<KeyValuePair> inheritance_by_gene, ReportVariantConfiguration& config, QWidget* parent)
	: QDialog(parent)
	, ui_()
	, config_(config)
{
	ui_.setupUi(this);
	ui_.variant->setText(variant);
	connect(ui_.type, SIGNAL(currentIndexChanged(int)) , this, SLOT(activateOkButtonIfValid()));

	//valid types
	QStringList types;
	types << "" << ReportVariantConfiguration::getTypeOptions();
	ui_.type->addItems(types);

	//valid inheritance options
	ui_.inheritance->addItems(ReportVariantConfiguration::getInheritanceModeOptions());

	//write settings if accepted
	connect(this, SIGNAL(accepted()), this, SLOT(writeBackSettings()));

	//set inheritance hint
	if (inheritance_by_gene.count()==0)
	{
		ui_.inheritance_hint->setVisible(false);
	}
	else
	{
		QString tooltip = "The following inheritance modes are set for the affected genes:";
		foreach(const KeyValuePair& pair, inheritance_by_gene)
		{
			tooltip += "\n" + pair.key + ": " + pair.value;
		}
		ui_.inheritance_hint->setToolTip(tooltip);
	}

	updateGUI();
}

void ReportVariantDialog::updateGUI()
{
	//data
	ui_.type->setCurrentText(config_.report_type);
	ui_.causal->setChecked(config_.causal);
	ui_.inheritance->setCurrentText(config_.inheritance);
	ui_.de_novo->setChecked(config_.de_novo);
	ui_.mosaic->setChecked(config_.mosaic);
	ui_.comp_het->setChecked(config_.comp_het);
	ui_.exclude_artefact->setChecked(config_.exclude_artefact);
	ui_.exclude_frequency->setChecked(config_.exclude_frequency);
	ui_.exclude_phenotype->setChecked(config_.exclude_phenotype);
	ui_.exclude_mechanism->setChecked(config_.exclude_mechanism);
	ui_.exclude_other->setChecked(config_.exclude_other);
	ui_.comments->setPlainText(config_.comments);
	ui_.comments2->setPlainText(config_.comments2);

	//buttons
	activateOkButtonIfValid();
}

void ReportVariantDialog::writeBackSettings()
{
	config_.report_type = ui_.type->currentText();
	config_.causal = ui_.causal->isChecked();
	config_.inheritance = ui_.inheritance->currentText();
	config_.de_novo = ui_.de_novo->isChecked();
	config_.mosaic = ui_.mosaic->isChecked();
	config_.comp_het = ui_.comp_het->isChecked();
	config_.exclude_artefact = ui_.exclude_artefact->isChecked();
	config_.exclude_frequency = ui_.exclude_frequency->isChecked();
	config_.exclude_phenotype = ui_.exclude_phenotype->isChecked();
	config_.exclude_mechanism = ui_.exclude_mechanism->isChecked();
	config_.exclude_other = ui_.exclude_other->isChecked();
	config_.comments = ui_.comments->toPlainText();
	config_.comments2 = ui_.comments2->toPlainText();
}

void ReportVariantDialog::activateOkButtonIfValid()
{
	//disable button
	ui_.buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);

	//check type
	QString type = ui_.type->currentText();
	QStringList valid_types = ReportVariantConfiguration::getTypeOptions();
	if (!valid_types.contains(type)) return;

	//enable button
	ui_.buttonBox->button(QDialogButtonBox::Ok)->setEnabled(true);
}
