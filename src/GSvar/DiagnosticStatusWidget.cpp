#include "DiagnosticStatusWidget.h"
#include "NGSD.h"

DiagnosticStatusWidget::DiagnosticStatusWidget(QWidget *parent)
	: QWidget(parent)
	, ui()
	, initial_status_text_()
{
	ui.setupUi(this);

	//init comboboxes from DB enums
	NGSD db;
	ui.status->addItems(db.getEnum("diag_status", "status"));
	ui.outcome->addItems(db.getEnum("diag_status", "outcome"));
	ui.inheritance->addItems(db.getEnum("diag_status", "inheritance_mode"));
	ui.evidence->addItems(db.getEnum("diag_status", "evidence_level"));

	//signals+slots
	connect(ui.outcome, SIGNAL(currentTextChanged(QString)), this, SIGNAL(outcomeChanged(QString)));
}

void DiagnosticStatusWidget::setStatus(DiagnosticStatusData data)
{
	QString last_edit = data.user + " / " + data.date.toString(Qt::ISODate).replace('T', ' ');
	if (last_edit.trimmed()=="/") last_edit.clear();
	ui.user_date->setText(last_edit);
	ui.status->setCurrentText(data.dagnostic_status);
	ui.outcome->setCurrentText(data.outcome);
	ui.genes_causal->setText(data.genes_causal);
	ui.inheritance->setCurrentText(data.inheritance_mode);
	ui.evidence->setCurrentText(data.evidence_level);
	ui.genes_incidental->setText(data.genes_incidental);
	ui.comment->setText(data.comments);

	//store initial status to check if sample is scheduled for re-sequencin
	initial_status_text_ = data.dagnostic_status;
}

DiagnosticStatusData DiagnosticStatusWidget::status() const
{
	DiagnosticStatusData output;

	output.dagnostic_status = ui.status->currentText();
	output.outcome = ui.outcome->currentText();
	output.genes_causal = ui.genes_causal->text().trimmed();
	output.inheritance_mode = ui.inheritance->currentText();
	output.evidence_level = ui.evidence->currentText();
	output.genes_incidental = ui.genes_incidental->text().trimmed();
	output.comments = ui.comment->text().trimmed();

	return output;
}

bool DiagnosticStatusWidget::resequencingRequested() const
{
	return ui.status->currentText().startsWith("repeat") && !initial_status_text_.startsWith("repeat");
}

