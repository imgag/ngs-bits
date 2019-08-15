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
	connect(ui_.inheritance, SIGNAL(currentIndexChanged(int)) , this, SLOT(activateOkButtonIfValid()));
	connect(ui_.type, SIGNAL(currentIndexChanged(int)) , this, SLOT(activateOkButtonIfValid()));

	//valid type/inheritance
	ui_.type->addItem("");
	QStringList types = ReportVariantConfiguration::getTypeOptions();
	foreach(QString type, types)
	{
		ReportVariantConfiguration tmp;
		tmp.type = type;
		ui_.type->addItem(tmp.icon(), type);
	}

	QStringList inheritance_modes;
	inheritance_modes << "" << ReportVariantConfiguration::getInheritanceModeOptions();
	ui_.inheritance->addItems(inheritance_modes);

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
	ui_.type->setCurrentText(config_.type);
	ui_.inheritance->setCurrentText(config_.inheritance_mode);
	ui_.de_novo->setChecked(config_.de_novo);
	ui_.mosaic->setChecked(config_.mosaic);
	ui_.comp_het->setChecked(config_.comp_het);
	ui_.comments->setPlainText(config_.comment);

	//buttons
	activateOkButtonIfValid();
}

void ReportVariantDialog::writeBackSettings()
{
	config_.type = ui_.type->currentText();
	config_.inheritance_mode = ui_.inheritance->currentText();
	config_.de_novo = ui_.de_novo->isChecked();
	config_.mosaic = ui_.mosaic->isChecked();
	config_.comp_het = ui_.comp_het->isChecked();
	config_.comment = ui_.comments->toPlainText();
}

void ReportVariantDialog::activateOkButtonIfValid()
{
	//disable button
	ui_.buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);

	//check inheritance
	QString inheritance = ui_.inheritance->currentText();
	QStringList valid_inheritance = ReportVariantConfiguration::getInheritanceModeOptions();
	if (!valid_inheritance.contains(inheritance)) return;

	//check type
	QString type = ui_.type->currentText();
	QStringList valid_types = ReportVariantConfiguration::getTypeOptions();
	if (!valid_types.contains(type)) return;

	//enable button
	ui_.buttonBox->button(QDialogButtonBox::Ok)->setEnabled(true);
}
