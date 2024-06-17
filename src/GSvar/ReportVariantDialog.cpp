#include "ReportVariantDialog.h"
#include "ClassificationDialog.h"
#include "Settings.h"
#include "VariantOpenDialog.h"
#include <QMessageBox>

ReportVariantDialog::ReportVariantDialog(QString variant, QList<KeyValuePair> inheritance_by_gene, ReportVariantConfiguration& config, QWidget* parent)
	: QDialog(parent)
	, ui_()
	, variant_(variant)
	, config_(config)
	, genome_idx_(Settings::string("reference_genome", false))
{
	ui_.setupUi(this);
	connect(ui_.manual_small_var_import, SIGNAL(clicked(bool)), this, SLOT(importManualSmallVariant()));
	ui_.variant->setText(variant_);

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
	foreach(QLineEdit* widget, findChildren<QLineEdit*>())
	{
		connect(widget, SIGNAL(textChanged(QString)), this, SLOT(activateOkButtonIfValid()));
	}

	//fill combo-boxes
	QStringList types;
	types << "" << ReportVariantConfiguration::getTypeOptions();
	ui_.type->addItems(types);
	ui_.inheritance->addItems(ReportVariantConfiguration::getInheritanceModeOptions());
	ui_.classification->addItems(ReportVariantConfiguration::getClassificationOptions());
	ui_.rna_info->addItems(ReportVariantConfiguration::getRnaInfoOptions());
	ui_.rna_info->setVisible(config.variant_type!=VariantType::RES);

	//show classification when needed
	bool show_classification = (config.variant_type==VariantType::CNVS || config.variant_type==VariantType::SVS);
	ui_.classification->setVisible(show_classification);
	ui_.label_classification->setVisible(show_classification);

	//show exclude resons only when needed
	ui_.exclude_frequency->setVisible(config.variant_type!=VariantType::RES);
	ui_.exclude_mechanism->setVisible(config.variant_type!=VariantType::RES);

	//show manual variant override options when needed
	foreach(QWidget* widget, findChildren<QWidget*>(QRegExp("manual_.*")))
	{
		widget->setVisible(false);
	}
	if (config_.variant_type==VariantType::SNVS_INDELS)
	{
		ui_.manual_line->setVisible(true);
		foreach(QWidget* widget, findChildren<QWidget*>(QRegExp("manual_.*small.*")))
		{
			widget->setVisible(true);
		}
	}
	if (config_.variant_type==VariantType::CNVS)
	{
		ui_.manual_line->setVisible(true);
		foreach(QWidget* widget, findChildren<QWidget*>(QRegExp("manual_.*cnv.*")))
		{
			widget->setVisible(true);
		}
	}
	if (config_.variant_type==VariantType::SVS)
	{
		ui_.manual_line->setVisible(true);
		foreach(QWidget* widget, findChildren<QWidget*>(QRegExp("manual_.*sv.*")))
		{
			widget->setVisible(true);
		}
		if (!variant_.startsWith("BND"))
		{
			ui_.manual_label_sv_start_bnd->setVisible(false);
			ui_.manual_label_sv_end_bnd->setVisible(false);
			ui_.manual_label_sv_hgvs_type_bnd->setVisible(false);
			ui_.manual_label_sv_hgvs_suffix_bnd->setVisible(false);

			ui_.manual_sv_start_bnd->setVisible(false);
			ui_.manual_sv_end_bnd->setVisible(false);
			ui_.manual_sv_hgvs_type_bnd->setVisible(false);
			ui_.manual_sv_hgvs_suffix_bnd->setVisible(false);
		}
	}
	if (config_.variant_type==VariantType::RES)
	{
		ui_.manual_line->setVisible(true);
		foreach(QWidget* widget, findChildren<QWidget*>(QRegExp("manual_.*re.*")))
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
	if (config_.variant_type!=VariantType::RES)
	{
		ui_.rna_info->setCurrentText(config_.rna_info);
	}

	//manual curation small variants
	if (config_.variant_type==VariantType::SNVS_INDELS && !config_.manual_var.trimmed().isEmpty())
	{
		ui_.manual_small_var->setText(config_.manual_var.trimmed());
		ui_.manual_small_genotype->setText(config_.manual_genotype.trimmed());
	}

	//manual curation CNVs
	if (config_.variant_type==VariantType::CNVS)
	{
		ui_.manual_cnv_start->setText(config_.manual_cnv_start.trimmed());
		ui_.manual_cnv_end->setText(config_.manual_cnv_end.trimmed());
		ui_.manual_cnv_cn->setText(config_.manual_cnv_cn.trimmed());
		ui_.manual_cnv_hgvs_type->setText(config_.manual_cnv_hgvs_type.trimmed());
		ui_.manual_cnv_hgvs_suffix->setText(config_.manual_cnv_hgvs_suffix.trimmed());
	}

	//manual curation SVs
	if (config_.variant_type==VariantType::SVS)
	{
		ui_.manual_sv_start->setText(config_.manual_sv_start.trimmed());
		ui_.manual_sv_end->setText(config_.manual_sv_end.trimmed());
		ui_.manual_sv_genotype->setText(config_.manual_sv_genotype.trimmed());
		ui_.manual_sv_start_bnd->setText(config_.manual_sv_start_bnd.trimmed());
		ui_.manual_sv_end_bnd->setText(config_.manual_sv_end_bnd.trimmed());
		ui_.manual_sv_hgvs_type->setText(config_.manual_sv_hgvs_type.trimmed());
		ui_.manual_sv_hgvs_suffix->setText(config_.manual_sv_hgvs_suffix.trimmed());
		ui_.manual_sv_hgvs_type_bnd->setText(config_.manual_sv_hgvs_type_bnd.trimmed());
		ui_.manual_sv_hgvs_suffix_bnd->setText(config_.manual_sv_hgvs_suffix_bnd.trimmed());
	}

	//manual curation REs
	if (config_.variant_type==VariantType::SVS)
	{
		ui_.manual_re_allele1->setText(config_.manual_re_allele1.trimmed());
		ui_.manual_re_allele2->setText(config_.manual_re_allele2.trimmed());
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
	if (config_.variant_type!=VariantType::RES)
	{
		rvc.rna_info = ui_.rna_info->currentText();
	}

	//manual curation small variants
	if (rvc.variant_type==VariantType::SNVS_INDELS)
	{
		rvc.manual_var = ui_.manual_small_var->text().trimmed(); //not ok > error in validation
		rvc.manual_genotype = ui_.manual_small_genotype->text().trimmed(); //not ok > error in validation
	}

	//manual curation CNVs
	if (rvc.variant_type==VariantType::CNVS)
	{
		rvc.manual_cnv_start = ui_.manual_cnv_start->text().trimmed();
		rvc.manual_cnv_end = ui_.manual_cnv_end->text().trimmed();
		rvc.manual_cnv_cn = ui_.manual_cnv_cn->text().trimmed();
		rvc.manual_cnv_hgvs_type = ui_.manual_cnv_hgvs_type->text().trimmed();
		rvc.manual_cnv_hgvs_suffix = ui_.manual_cnv_hgvs_suffix->text().trimmed();
	}

	//manual curation SVs
	if (rvc.variant_type==VariantType::SVS)
	{
		rvc.manual_sv_start = ui_.manual_sv_start->text().trimmed();
		rvc.manual_sv_end = ui_.manual_sv_end->text().trimmed();
		rvc.manual_sv_genotype = ui_.manual_sv_genotype->text().trimmed();
		rvc.manual_sv_start_bnd = ui_.manual_sv_start_bnd->text().trimmed();
		rvc.manual_sv_end_bnd = ui_.manual_sv_end_bnd->text().trimmed();
		rvc.manual_sv_hgvs_type = ui_.manual_sv_hgvs_type->text().trimmed();
		rvc.manual_sv_hgvs_suffix = ui_.manual_sv_hgvs_suffix->text().trimmed();
		rvc.manual_sv_hgvs_type_bnd = ui_.manual_sv_hgvs_type_bnd->text().trimmed();
		rvc.manual_sv_hgvs_suffix_bnd = ui_.manual_sv_hgvs_suffix_bnd->text().trimmed();
	}

	//manual curation REs
	if (rvc.variant_type==VariantType::RES)
	{
		rvc.manual_re_allele1 = ui_.manual_re_allele1->text().trimmed();
		rvc.manual_re_allele2 = ui_.manual_re_allele2->text().trimmed();
	}
}

void ReportVariantDialog::activateOkButtonIfValid()
{
	QStringList errors;

	//check if config is valid
	ReportVariantConfiguration tmp = config_;
	writeBack(tmp);
	if (!tmp.isValid(errors, genome_idx_))
	{
		ui_.error_label->setVisible(true);
		ui_.error_label->setToolTip(errors.join("\n"));
		ui_.btn_ok->setEnabled(false);
		return;
	}

	//special handling for manual curation of small variant: check that new coordinates are nearby
	if (tmp.variant_type==VariantType::SNVS_INDELS && tmp.manualVarIsValid(genome_idx_))
	{
		Variant v = Variant::fromString(variant_);
		Variant v2 = Variant::fromString(tmp.manual_var);
		if (v.chr()!=v2.chr() || abs(v.start()-v2.start())>1000)
		{
			ui_.error_label->setVisible(true);
			ui_.error_label->setToolTip("Manual coordinates are too far from original coordinates!");
			ui_.btn_ok->setEnabled(false);
			return;
		}
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

void ReportVariantDialog::importManualSmallVariant()
{
	VariantOpenDialog dlg(this);
	dlg.setDefaultFormat("HGVS.c");
	if (dlg.exec()!=QDialog::Accepted) return;

	try
	{
		Variant v = dlg.variant();
		ui_.manual_small_var->setText(v.toString());
	}
	catch(Exception& e)
	{
		QMessageBox::information(this, "Manual variant curation", "Variant is not valid:\n" + e.message());
	}
}
