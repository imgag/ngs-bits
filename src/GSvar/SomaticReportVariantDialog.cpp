#include "SomaticReportVariantDialog.h"
#include "ui_SomaticReportVariantDialog.h"
#include "NGSD.h"
#include <QPushButton>

SomaticReportVariantDialog::SomaticReportVariantDialog(QString variant, SomaticReportVariantConfiguration& var_conf, QWidget *parent)
	: QDialog(parent)
	, ui_()
	, var_conf_(var_conf)
{
	ui_.setupUi(this);
	ui_.variant->setText(variant);

	connect(ui_.exclude_artefact, SIGNAL(stateChanged(int)), this, SLOT(activateOkButtonIfValid()));
	connect(ui_.exclude_high_baf_deviation, SIGNAL(stateChanged(int)), this, SLOT(activateOkButtonIfValid()));
	connect(ui_.exclude_low_cn, SIGNAL(stateChanged(int)), this, SLOT(activateOkButtonIfValid()));
	connect(ui_.exclude_low_tumor_content, SIGNAL(stateChanged(int)), this, SLOT(activateOkButtonIfValid()));
	connect(ui_.exclude_other, SIGNAL(stateChanged(int)), this, SLOT(activateOkButtonIfValid()));
	connect(ui_.exclude_unclear_effect, SIGNAL(stateChanged(int)), this, SLOT(activateOkButtonIfValid()));

	connect(ui_.include_variant_alt, SIGNAL(textEdited(QString)), this, SLOT(activateOkButtonIfValid()));
	connect(ui_.include_variant_desc, SIGNAL(textEdited(QString)), this, SLOT(activateOkButtonIfValid()));

	connect(ui_.manual_sv_start, SIGNAL(textEdited(QString)), this, SLOT(activateOkButtonIfValid()));
	connect(ui_.manual_sv_end, SIGNAL(textEdited(QString)), this, SLOT(activateOkButtonIfValid()));
	connect(ui_.manual_sv_hgvs_type, SIGNAL(textEdited(QString)), this, SLOT(activateOkButtonIfValid()));
	connect(ui_.manual_sv_hgvs_suffix, SIGNAL(textEdited(QString)), this, SLOT(activateOkButtonIfValid()));

	connect(ui_.manual_sv_start_bnd, SIGNAL(textEdited(QString)), this, SLOT(activateOkButtonIfValid()));
	connect(ui_.manual_sv_end_bnd, SIGNAL(textEdited(QString)), this, SLOT(activateOkButtonIfValid()));
	connect(ui_.manual_sv_hgvs_type_bnd, SIGNAL(textEdited(QString)), this, SLOT(activateOkButtonIfValid()));
	connect(ui_.manual_sv_hgvs_suffix_bnd, SIGNAL(textEdited(QString)), this, SLOT(activateOkButtonIfValid()));

	connect(ui_.description_sv, SIGNAL(textChanged()), this, SLOT(activateOkButtonIfValid()));

	//write settings if accepted
	connect(this, SIGNAL(accepted()), this, SLOT(writeBackSettings()));

	//hide sv override options
    foreach(QWidget* widget, findChildren<QWidget*>(QRegularExpression("manual_sv.*")))
	{
		widget->setVisible(false);
	}
	ui_.exclude_unclear_effect->setVisible(false);
	ui_.sv_line_1->setVisible(false);
	ui_.sv_line_2->setVisible(false);

	ui_.rna_info->setVisible(false);
	ui_.rna_info_label->setVisible(false);
	ui_.description_sv->setVisible(false);
	ui_.description_sv_label->setVisible(false);

	if (var_conf_.variant_type==VariantType::SVS)
	{
		//hide parts not relevant to SVs
		ui_.exclude_high_baf_deviation->setVisible(false);
		ui_.exclude_low_cn->setVisible(false);
		ui_.exclude_low_tumor_content->setVisible(false);
		ui_.frame->setVisible(false);
		ui_.include_label->setVisible(false);
		ui_.line->setVisible(false);

		//show parts specifically relevant to SVs
		ui_.sv_line_1->setVisible(true);
		ui_.sv_line_2->setVisible(true);
		ui_.rna_info->setVisible(true);
		ui_.rna_info_label->setVisible(true);
		ui_.description_sv->setVisible(true);
		ui_.description_sv_label->setVisible(true);
		ui_.exclude_unclear_effect->setVisible(true);


        foreach(QWidget* widget, findChildren<QWidget*>(QRegularExpression("manual_sv.*")))
		{
			widget->setVisible(true);
		}
		if (! ui_.variant->text().startsWith("BND"))
		{
			ui_.manual_sv_start_bnd_label->setVisible(false);
			ui_.manual_sv_end_bnd_label->setVisible(false);
			ui_.manual_sv_hgvs_type_bnd_label->setVisible(false);
			ui_.manual_sv_hgvs_suffix_bnd_label->setVisible(false);

			ui_.manual_sv_start_bnd->setVisible(false);
			ui_.manual_sv_end_bnd->setVisible(false);
			ui_.manual_sv_hgvs_type_bnd->setVisible(false);
			ui_.manual_sv_hgvs_suffix_bnd->setVisible(false);
		}
	}

	if (var_conf_.variant_type == VariantType::CNVS || var_conf_.variant_type == VariantType::SVS)
	{
		ui_.include_variant_alt->setEnabled(false);
		ui_.include_variant_desc->setEnabled(false);
	}

	updateGUI();
	activateOkButtonIfValid();
}

void SomaticReportVariantDialog::updateGUI()
{
	ui_.exclude_artefact->setChecked(var_conf_.exclude_artefact);
	ui_.exclude_high_baf_deviation->setChecked(var_conf_.exclude_high_baf_deviation);
	ui_.exclude_low_cn->setChecked(var_conf_.exclude_low_copy_number);
	ui_.exclude_low_tumor_content->setChecked(var_conf_.exclude_low_tumor_content);
	ui_.exclude_other->setChecked(var_conf_.exclude_other_reason);
	ui_.exclude_unclear_effect->setChecked(var_conf_.exclude_unclear_effect);
	ui_.include_variant_alt->setText(var_conf_.include_variant_alteration);
	ui_.include_variant_desc->setText(var_conf_.include_variant_description);
	ui_.comment->setPlainText(var_conf_.comment);

	ui_.description_sv->setPlainText(var_conf_.description);

	ui_.manual_sv_start->setText(var_conf_.manual_sv_start);
	ui_.manual_sv_end->setText(var_conf_.manual_sv_end);
	ui_.manual_sv_hgvs_type->setText(var_conf_.manual_sv_hgvs_type);
	ui_.manual_sv_hgvs_suffix->setText(var_conf_.manual_sv_hgvs_suffix);

	ui_.manual_sv_start_bnd->setText(var_conf_.manual_sv_start);
	ui_.manual_sv_end_bnd->setText(var_conf_.manual_sv_end);
	ui_.manual_sv_hgvs_type_bnd->setText(var_conf_.manual_sv_hgvs_type_bnd);
	ui_.manual_sv_hgvs_suffix_bnd->setText(var_conf_.manual_sv_hgvs_suffix_bnd);

	ui_.rna_info->addItems(NGSD().getEnum("somatic_report_configuration_sv", "rna_info"));
	ui_.rna_info->setCurrentText(var_conf_.rna_info.isEmpty() ? "n/a" : var_conf_.rna_info);
}

void SomaticReportVariantDialog::activateOkButtonIfValid()
{
	ui_.buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);

	//include entry and exclude checkboxes cannot be set at the same time
	bool exclude_is_checked = ui_.exclude_artefact->isChecked() || ui_.exclude_high_baf_deviation->isChecked() || ui_.exclude_low_cn->isChecked() || ui_.exclude_low_tumor_content->isChecked() || ui_.exclude_other->isChecked() || ui_.exclude_unclear_effect->isChecked();
	bool entry_included = !ui_.include_variant_alt->text().trimmed().isEmpty() || !ui_.include_variant_desc->text().trimmed().isEmpty() || !ui_.comment->toPlainText().trimmed().isEmpty();
	bool entry_sv_included = !ui_.description_sv->toPlainText().trimmed().isEmpty() || !ui_.manual_sv_start->text().trimmed().isEmpty() || !ui_.manual_sv_end->text().trimmed().isEmpty() || !ui_.manual_sv_hgvs_type->text().trimmed().isEmpty() || !ui_.manual_sv_hgvs_suffix->text().trimmed().isEmpty();
	bool entry_sv_bnd_included = !ui_.manual_sv_start_bnd->text().trimmed().isEmpty() || !ui_.manual_sv_end_bnd->text().trimmed().isEmpty() || !ui_.manual_sv_hgvs_type_bnd->text().trimmed().isEmpty() || !ui_.manual_sv_hgvs_suffix_bnd->text().trimmed().isEmpty();
	if(exclude_is_checked && (entry_included || entry_sv_included || entry_sv_bnd_included)) return;
	if(!exclude_is_checked && ! (entry_included || entry_sv_included || entry_sv_bnd_included)) return;

	ui_.buttonBox->button(QDialogButtonBox::Ok)->setEnabled(true);
}

void SomaticReportVariantDialog::writeBackSettings()
{
	var_conf_.exclude_artefact = ui_.exclude_artefact->isChecked();
	var_conf_.exclude_high_baf_deviation = ui_.exclude_high_baf_deviation->isChecked();
	var_conf_.exclude_low_copy_number = ui_.exclude_low_cn->isChecked();
	var_conf_.exclude_low_tumor_content = ui_.exclude_low_tumor_content->isChecked();
	var_conf_.exclude_other_reason = ui_.exclude_other->isChecked();
	var_conf_.exclude_unclear_effect = ui_.exclude_unclear_effect->isChecked();

	var_conf_.include_variant_alteration = ui_.include_variant_alt->text().trimmed();
	var_conf_.include_variant_description = ui_.include_variant_desc->text().trimmed();

	var_conf_.comment = ui_.comment->toPlainText().trimmed();
	var_conf_.description = ui_.description_sv->toPlainText().trimmed();

	var_conf_.manual_sv_start = ui_.manual_sv_start->text();
	var_conf_.manual_sv_end = ui_.manual_sv_end->text();
	var_conf_.manual_sv_hgvs_type = ui_.manual_sv_hgvs_type->text();
	var_conf_.manual_sv_hgvs_suffix = ui_.manual_sv_hgvs_suffix->text();

	var_conf_.manual_sv_start_bnd = ui_.manual_sv_start_bnd->text();
	var_conf_.manual_sv_end_bnd = ui_.manual_sv_end_bnd->text();
	var_conf_.manual_sv_hgvs_type_bnd = ui_.manual_sv_hgvs_type_bnd->text();
	var_conf_.manual_sv_hgvs_suffix_bnd = ui_.manual_sv_hgvs_suffix_bnd->text();

	var_conf_.rna_info = ui_.rna_info->currentText();
}
