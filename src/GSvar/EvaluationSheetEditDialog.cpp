#include "EvaluationSheetEditDialog.h"
#include "ui_EvaluationSheetEditDialog.h"

#include <QMessageBox>

EvaluationSheetEditDialog::EvaluationSheetEditDialog(QWidget *parent) :
	QDialog(parent),
	ui_(new Ui::EvaluationSheetEditDialog)
{
	ui_->setupUi(this);
	connect(ui_->buttonBox, SIGNAL(accepted()), this, SLOT(updateEvaluationSheetData()));
	connect(ui_->ds_reviewer1, SIGNAL(textChanged(QString)), this, SLOT(checkReviewer()));
	connect(ui_->ds_reviewer2, SIGNAL(textChanged(QString)), this, SLOT(checkReviewer()));
	connect(ui_->le_dna_rna, SIGNAL(textChanged(QString)), this, SLOT(checkNonEmpty()));
	connect(ui_->le_analysis_scope, SIGNAL(textChanged(QString)), this, SLOT(checkNonEmpty()));
	connect(ui_->le_settlement_volume, SIGNAL(textChanged(QString)), this, SLOT(checkNonEmpty()));
	connect(this, SIGNAL(EvaluationSheetDataUpdated()), this, SLOT(accept()));
	initReviewerNames();

}

EvaluationSheetEditDialog::~EvaluationSheetEditDialog()
{
	delete ui_;
}

void EvaluationSheetEditDialog::importEvaluationSheetData(EvaluationSheetData& evaluation_sheet_data)
{
	evaluation_sheet_data_ = &evaluation_sheet_data;

	// update GUI
	ui_->le_dna_rna->setText(evaluation_sheet_data_->dna_rna);
	if (db_.userId(evaluation_sheet_data_->reviewer1, false, false) != -1) ui_->ds_reviewer1->setText(evaluation_sheet_data_->reviewer1);
	ui_->de_review1->setDate(evaluation_sheet_data_->review_date1);
	if (db_.userId(evaluation_sheet_data_->reviewer2, false, false) != -1) ui_->ds_reviewer2->setText(evaluation_sheet_data_->reviewer2);
	ui_->de_review2->setDate(evaluation_sheet_data_->review_date2);

	ui_->le_analysis_scope->setText(evaluation_sheet_data_->analysis_scope);
	ui_->le_settlement_volume->setText(evaluation_sheet_data_->settlement_volume);
	ui_->cb_acmg_requested->setChecked(evaluation_sheet_data_->acmg_requested);
	ui_->cb_acmg_analyzed->setChecked(evaluation_sheet_data_->acmg_analyzed);
	ui_->cb_acmg_suspicious->setChecked(evaluation_sheet_data_->acmg_noticeable);


	ui_->cb_filter_freq_dom->setChecked(evaluation_sheet_data_->filtered_by_freq_based_dominant);
	ui_->cb_filter_rec->setChecked(evaluation_sheet_data_->filtered_by_freq_based_recessive);
	ui_->cb_filter_cnv->setChecked(evaluation_sheet_data_->filtered_by_cnv);
	ui_->cb_filter_mito->setChecked(evaluation_sheet_data_->filtered_by_mito);
	ui_->cb_filter_x_chr->setChecked(evaluation_sheet_data_->filtered_by_x_chr);
	ui_->cb_filter_phen->setChecked(evaluation_sheet_data_->filtered_by_phenotype);
	ui_->cb_filter_multi->setChecked(evaluation_sheet_data_->filtered_by_multisample);
}

void EvaluationSheetEditDialog::updateEvaluationSheetData()
{
	QStringList error_messages;
	// check reviewer names
	if (!ui_->ds_reviewer1->isValidSelection()) error_messages.append("Ungültiger Name für Auswerter 1!");
	if (!ui_->ds_reviewer2->isValidSelection()) error_messages.append("Ungültiger Name für Auswerter 2!");
	// check QLineEdit fields for empty text
	if (ui_->le_dna_rna->text().trimmed() == "") error_messages.append("Ungültige DNA/RNA-Nummer!");
	if (ui_->le_analysis_scope->text().trimmed() == "") error_messages.append("Ungültiger Auswerteumfang!");
	if (ui_->le_settlement_volume->text().trimmed() == "") error_messages.append("Ungültiger Abrechnungsumfang!");

	if (error_messages.size() != 0)
	{
		QMessageBox::warning(this, "Input error", "Fehler: \n" + error_messages.join("\n"));
		return;
	}
	// read in date from gui
	evaluation_sheet_data_->dna_rna = ui_->le_dna_rna->text();
	evaluation_sheet_data_->reviewer1 = ui_->ds_reviewer1->text();
	evaluation_sheet_data_->review_date1 = ui_->de_review1->date();
	evaluation_sheet_data_->reviewer2 = ui_->ds_reviewer2->text();
	evaluation_sheet_data_->review_date2 = ui_->de_review2->date();

	evaluation_sheet_data_->analysis_scope = ui_->le_analysis_scope->text();
	evaluation_sheet_data_->settlement_volume = ui_->le_settlement_volume->text();
	evaluation_sheet_data_->acmg_requested = ui_->cb_acmg_requested->isChecked();
	evaluation_sheet_data_->acmg_analyzed = ui_->cb_acmg_analyzed->isChecked();
	evaluation_sheet_data_->acmg_noticeable = ui_->cb_acmg_suspicious->isChecked();


	evaluation_sheet_data_->filtered_by_freq_based_dominant = ui_->cb_filter_freq_dom->isChecked();
	evaluation_sheet_data_->filtered_by_freq_based_recessive = ui_->cb_filter_rec->isChecked();
	evaluation_sheet_data_->filtered_by_cnv = ui_->cb_filter_cnv->isChecked();
	evaluation_sheet_data_->filtered_by_mito = ui_->cb_filter_mito->isChecked();
	evaluation_sheet_data_->filtered_by_x_chr = ui_->cb_filter_x_chr->isChecked();
	evaluation_sheet_data_->filtered_by_phenotype = ui_->cb_filter_phen->isChecked();
	evaluation_sheet_data_->filtered_by_multisample = ui_->cb_filter_multi->isChecked();

	emit EvaluationSheetDataUpdated();

}

void EvaluationSheetEditDialog::checkReviewer()
{
	// try to cast to QLineEdit:
	DBSelector* db_selector = qobject_cast<DBSelector*> (sender());
	if (db_selector == NULL) return;

	// check if reviewer name is valid
	if (db_selector->isValidSelection()) db_selector->setStyleSheet("");
	else db_selector->setStyleSheet("QLineEdit { background: rgba(255, 0, 0, 64); }");
}

void EvaluationSheetEditDialog::checkNonEmpty()
{
	// try to cast to QLineEdit:
	QLineEdit* line_edit = qobject_cast<QLineEdit*> (sender());
	if (line_edit == NULL) return;

	// set background to red if text is empty
	if (line_edit->text().trimmed() == "") line_edit->setStyleSheet("QLineEdit { background: rgba(255, 0, 0, 64); }");
	else line_edit->setStyleSheet("");
}

void EvaluationSheetEditDialog::initReviewerNames()
{
	users_ = db_.createTable("users", "SELECT id, name FROM user");
	ui_->ds_reviewer1->fill(users_);
	ui_->ds_reviewer2->fill(users_);
}
