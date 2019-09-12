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
	connect(ui_.report, SIGNAL(currentIndexChanged(int)) , this, SLOT(activateOkButtonIfValid()));
	connect(ui_.type, SIGNAL(currentIndexChanged(int)) , this, SLOT(activateOkButtonIfValid()));

	//valid report options
	ui_.report->addItem("");
	QStringList report_options = ReportVariantConfiguration::getReportOptions();
	foreach(QString option, report_options)
	{
		ReportVariantConfiguration tmp;
		tmp.report = option;
		ui_.report->addItem(tmp.icon(), option);
	}

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
	ui_.report->setCurrentText(config_.report);
	ui_.type->setCurrentText(config_.type);
	ui_.causal->setChecked(config_.causal);
	ui_.inheritance->setCurrentText(config_.inheritance_mode);
	ui_.de_novo->setChecked(config_.de_novo);
	ui_.mosaic->setChecked(config_.mosaic);
	ui_.comp_het->setChecked(config_.comp_het);
	ui_.comments->setPlainText(config_.comment);
	ui_.comments2->setPlainText(config_.comment2);

	//buttons
	activateOkButtonIfValid();
}

void ReportVariantDialog::writeBackSettings()
{
	config_.report = ui_.report->currentText();
	config_.type = ui_.type->currentText();
	config_.causal = ui_.causal->isChecked();
	config_.inheritance_mode = ui_.inheritance->currentText();
	config_.de_novo = ui_.de_novo->isChecked();
	config_.mosaic = ui_.mosaic->isChecked();
	config_.comp_het = ui_.comp_het->isChecked();
	config_.comment = ui_.comments->toPlainText();
	config_.comment2 = ui_.comments2->toPlainText();
}

void ReportVariantDialog::activateOkButtonIfValid()
{
	//disable button
	ui_.buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);

	//check report
	QString report = ui_.report->currentText();
	QStringList report_options = ReportVariantConfiguration::getReportOptions();
	if (!report_options.contains(report)) return;

	//check type
	QString type = ui_.type->currentText();
	QStringList valid_types = ReportVariantConfiguration::getTypeOptions();
	if (!valid_types.contains(type)) return;

	//enable button
	ui_.buttonBox->button(QDialogButtonBox::Ok)->setEnabled(true);
}
