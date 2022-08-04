#include "ReportVariantDialog.h"
#include "ClassificationDialog.h"
#include "Settings.h"
#include <QPushButton>

ReportVariantDialog::ReportVariantDialog(QString variant, QList<KeyValuePair> inheritance_by_gene, ReportVariantConfiguration& config, QWidget* parent)
	: QDialog(parent)
	, ui_()
	, config_(config)
{
	ui_.setupUi(this);
	ui_.variant->setText(variant);

	//connect signals to enable/disable 'Ok' button
	foreach(QComboBox* widget, findChildren<QComboBox*>())
	{
		connect(widget, SIGNAL(currentIndexChanged(int)) , this, SLOT(activateOkButtonIfValid()));
	}
	foreach(QCheckBox* widget, findChildren<QCheckBox*>())
	{
		connect(widget, SIGNAL(stateChanged(int)) , this, SLOT(activateOkButtonIfValid()));
	}
	foreach(QPlainTextEdit* widget, findChildren<QPlainTextEdit*>())
	{
		connect(widget, SIGNAL(textChanged()), this, SLOT(activateOkButtonIfValid()));
	}

	//fill combo-boxes
	QStringList types;
	types << "" << ReportVariantConfiguration::getTypeOptions();
	ui_.type->addItems(types);
	ui_.inheritance->addItems(ReportVariantConfiguration::getInheritanceModeOptions());
	ui_.classification->addItems(ReportVariantConfiguration::getClassificationOptions());
	ui_.rna_info->addItems(ReportVariantConfiguration::getRnaInfoOptions());

	//show classification when needed
	bool show_classification = (config.variant_type==VariantType::CNVS || config.variant_type==VariantType::SVS);
	ui_.classification->setVisible(show_classification);
	ui_.label_classification->setVisible(show_classification);

	//show manual variant override options when needed
	foreach(QWidget* widget, findChildren<QWidget*>(QRegExp("manual_.*")))
	{
		widget->setVisible(false);
	}
	if (config_.variant_type==VariantType::CNVS)
	{
		ui_.manual_line->setVisible(true);
		foreach(QWidget* widget, findChildren<QWidget*>(QRegExp("manual_.*cnv.*")))
		{
			widget->setVisible(true);
		}
	}

	//write settings if accepted
	connect(this, SIGNAL(accepted()), this, SLOT(writeBackSettings()));

	//set inheritance hint
	QSet<QString> distinct_modes;
	foreach(const KeyValuePair& pair, inheritance_by_gene)
	{
		distinct_modes << pair.value;
	}
	if (distinct_modes.count()==0)
	{
		ui_.inheritance_hint->setVisible(false);
	}
	else //several options > let user select
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
	ui_.classification->setCurrentText(config_.classification);
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
	ui_.rna_info->setCurrentText(config_.rna_info);

	//manual curation CNVs
	if (config_.variant_type==VariantType::CNVS)
	{
		qDebug() << config_.manual_cnv_start.type() << config_.manual_cnv_start.isValid() << config_.manual_cnv_start.toString();
		ui_.manual_cnv_start->setText(config_.manual_cnv_start.toString());
		ui_.manual_cnv_end->setText(config_.manual_cnv_end.toString());
	}
}

bool ReportVariantDialog::variantReportConfigChanged()
{
	ReportVariantConfiguration tmp = config_;
	writeBack(tmp);
	return tmp!=config_;
}

void ReportVariantDialog::writeBackSettings()
{
	writeBack(config_);
}


void ReportVariantDialog::writeBack(ReportVariantConfiguration& rvc)
{
	rvc.report_type = ui_.type->currentText();
	rvc.causal = ui_.causal->isChecked();
	rvc.classification = ui_.classification->currentText();
	rvc.inheritance = ui_.inheritance->currentText();
	rvc.de_novo = ui_.de_novo->isChecked();
	rvc.mosaic = ui_.mosaic->isChecked();
	rvc.comp_het = ui_.comp_het->isChecked();
	rvc.exclude_artefact = ui_.exclude_artefact->isChecked();
	rvc.exclude_frequency = ui_.exclude_frequency->isChecked();
	rvc.exclude_phenotype = ui_.exclude_phenotype->isChecked();
	rvc.exclude_mechanism = ui_.exclude_mechanism->isChecked();
	rvc.exclude_other = ui_.exclude_other->isChecked();
	rvc.comments = ui_.comments->toPlainText();
	rvc.comments2 = ui_.comments2->toPlainText();
	rvc.rna_info = ui_.rna_info->currentText();

	//manual curation CNVs
	if (rvc.variant_type==VariantType::CNVS)
	{
		QString start_text = ui_.manual_cnv_start->text().trimmed();
		qDebug() << __LINE__ << start_text;
		if (!start_text.isEmpty())
		{
			bool ok = false;
			int start = start_text.toInt(&ok);
			qDebug() << __LINE__ << start;
			if (ok) rvc.manual_cnv_start = ok ? QVariant(start) : QVariant(start_text); //not ok > error in validation because of wrong QVariant type
			qDebug() << __LINE__ << rvc.manual_cnv_start;
		}

		QString end_text = ui_.manual_cnv_end->text().trimmed();
		if (!end_text.isEmpty())
		{
			bool ok = false;
			int end = end_text.toInt(&ok);
			rvc.manual_cnv_end = ok ? QVariant(end) : QVariant(end_text); //not ok > error in validation of wrong QVariant type
		}
	}
}

void ReportVariantDialog::activateOkButtonIfValid()
{
	QStringList errors;

	//check if config is valid
	ReportVariantConfiguration tmp = config_;
	writeBack(tmp);
	if (!tmp.isValid(errors))
	{
		ui_.error_label->setVisible(true);
		ui_.error_label->setToolTip(errors.join("\n"));
		ui_.btn_ok->setEnabled(false);
		return;
	}

	//check if data was changed (storing may override the changes someone else made in another instance of GSvar, thus avoid unncessary storing...)
	if (!variantReportConfigChanged())
	{
		ui_.error_label->setVisible(false);
		ui_.btn_ok->setEnabled(false);
		return;
	}

	//enable button
	ui_.error_label->setVisible(false);
	ui_.btn_ok->setEnabled(true);
}
