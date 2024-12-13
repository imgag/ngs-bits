#include "RnaReportFusionDialog.h"
#include "ui_RnaReportFusionDialog.h"
#include <QPushButton>
#include "NGSD.h"

RnaReportFusionDialog::RnaReportFusionDialog(QString variant, RnaReportFusionConfiguration& var_conf, QWidget *parent)
	: QDialog(parent)
	, ui_()
	, var_conf_(var_conf)
{
	ui_.setupUi(this);
	ui_.fusion->setText(variant);

	//write settings if accepted
	connect(this, SIGNAL(accepted()), this, SLOT(writeBackSettings()));

	updateGUI();
}

void RnaReportFusionDialog::updateGUI()
{
	ui_.exclude_artefact->setChecked(var_conf_.exclude_artefact);
	ui_.exclude_low_tumor_content->setChecked(var_conf_.exclude_low_tumor_content);
	ui_.exclude_other->setChecked(var_conf_.exclude_other_reason);
	ui_.exclude_low_evidence->setChecked(var_conf_.exclude_low_evidence);
	ui_.comment->setPlainText(var_conf_.comment);
}

void RnaReportFusionDialog::writeBackSettings()
{
	var_conf_.exclude_artefact = ui_.exclude_artefact->isChecked();
	var_conf_.exclude_low_tumor_content = ui_.exclude_low_tumor_content->isChecked();
	var_conf_.exclude_other_reason = ui_.exclude_other->isChecked();
	var_conf_.exclude_low_evidence = ui_.exclude_low_evidence->isChecked();

	var_conf_.comment = ui_.comment->toPlainText().trimmed();
}
