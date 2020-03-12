#include "SomaticReportVariantDialog.h"
#include "ui_SomaticReportVariantDialog.h"
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

	connect(ui_.include_variant_alt, SIGNAL(textEdited(QString)), this, SLOT(activateOkButtonIfValid()));
	connect(ui_.include_variant_desc, SIGNAL(textEdited(QString)), this, SLOT(activateOkButtonIfValid()));

	//write settings if accepted
	connect(this, SIGNAL(accepted()), this, SLOT(writeBackSettings()));

	updateGUI();
}

void SomaticReportVariantDialog::disableIncludeForm()
{
	ui_.include_variant_alt->setEnabled(false);
	ui_.include_variant_desc->setEnabled(false);
}


void SomaticReportVariantDialog::updateGUI()
{
	ui_.exclude_artefact->setChecked(var_conf_.exclude_artefact);
	ui_.exclude_high_baf_deviation->setChecked(var_conf_.exclude_high_baf_deviation);
	ui_.exclude_low_cn->setChecked(var_conf_.exclude_low_copy_number);
	ui_.exclude_low_tumor_content->setChecked(var_conf_.exclude_low_tumor_content);
	ui_.exclude_other->setChecked(var_conf_.exclude_other_reason);
	ui_.include_variant_alt->setText(var_conf_.include_variant_alteration);
	ui_.include_variant_desc->setText(var_conf_.include_variant_description);
	ui_.comment->setPlainText(var_conf_.comment);
}

void SomaticReportVariantDialog::activateOkButtonIfValid()
{
	ui_.buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);

	//include entry and exclude checkboxes cannot be set at the same time
	bool exclude_is_checked = ui_.exclude_artefact->isChecked() || ui_.exclude_high_baf_deviation->isChecked() || ui_.exclude_low_cn->isChecked() || ui_.exclude_low_tumor_content->isChecked() || ui_.exclude_other->isChecked();
	bool entry_included = !ui_.include_variant_alt->text().trimmed().isEmpty() || !ui_.include_variant_desc->text().trimmed().isEmpty();
	if(exclude_is_checked && entry_included) return;
	if(!exclude_is_checked && !entry_included) return;

	ui_.buttonBox->button(QDialogButtonBox::Ok)->setEnabled(true);
}

void SomaticReportVariantDialog::writeBackSettings()
{
	var_conf_.exclude_artefact = ui_.exclude_artefact->isChecked();
	var_conf_.exclude_high_baf_deviation = ui_.exclude_high_baf_deviation->isChecked();
	var_conf_.exclude_low_copy_number = ui_.exclude_low_cn->isChecked();
	var_conf_.exclude_low_tumor_content = ui_.exclude_low_tumor_content->isChecked();
	var_conf_.exclude_other_reason = ui_.exclude_other->isChecked();

	var_conf_.include_variant_alteration = ui_.include_variant_alt->text().trimmed();
	var_conf_.include_variant_description = ui_.include_variant_desc->text().trimmed();

	var_conf_.comment = ui_.comment->toPlainText().trimmed();

}
