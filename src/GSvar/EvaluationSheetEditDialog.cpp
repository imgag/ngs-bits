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

	QString ps = db_.processedSampleName(evaluation_sheet_data_->ps_id);
	setWindowTitle(windowTitle() + " of " + ps);

	// update GUI
	ui_->le_dna_rna->setText(evaluation_sheet_data_->dna_rna);
	if (db_.userId(evaluation_sheet_data_->reviewer1, false, false) != -1) ui_->ds_reviewer1->setText(evaluation_sheet_data_->reviewer1);
	ui_->de_review1->setDate(evaluation_sheet_data_->review_date1);
	if (db_.userId(evaluation_sheet_data_->reviewer2, false, false) != -1) ui_->ds_reviewer2->setText(evaluation_sheet_data_->reviewer2);
	ui_->de_review2->setDate(evaluation_sheet_data_->review_date2);

	ui_->cb_analysis_scope->setCurrentText(evaluation_sheet_data_->analysis_scope);
	ui_->cb_acmg_requested->setChecked(evaluation_sheet_data_->acmg_requested);
	ui_->cb_acmg_analyzed->setChecked(evaluation_sheet_data_->acmg_analyzed);
	ui_->cb_acmg_suspicious->setChecked(evaluation_sheet_data_->acmg_noticeable);


	ui_->cb_filter_freq_dom->setChecked(evaluation_sheet_data_->filtered_by_freq_based_dominant);
	ui_->cb_filter_rec->setChecked(evaluation_sheet_data_->filtered_by_freq_based_recessive);
	ui_->cb_filter_mito->setChecked(evaluation_sheet_data_->filtered_by_mito);
	ui_->cb_filter_x_chr->setChecked(evaluation_sheet_data_->filtered_by_x_chr);
	ui_->cb_filter_cnv->setChecked(evaluation_sheet_data_->filtered_by_cnv);
	ui_->cb_filter_svs->setChecked(evaluation_sheet_data_->filtered_by_svs);
	ui_->cb_filter_res->setChecked(evaluation_sheet_data_->filtered_by_res);
	ui_->cb_filter_mosaics->setChecked(evaluation_sheet_data_->filtered_by_mosaic);
	ui_->cb_filter_phen->setChecked(evaluation_sheet_data_->filtered_by_phenotype);
	ui_->cb_filter_multi->setChecked(evaluation_sheet_data_->filtered_by_multisample);
	ui_->cb_filter_trio_stringent->setChecked(evaluation_sheet_data_->filtered_by_trio_stringent);
	ui_->cb_filter_trio_relaxed->setChecked(evaluation_sheet_data_->filtered_by_trio_relaxed);
}

void EvaluationSheetEditDialog::updateEvaluationSheetData()
{
	QStringList error_messages;
	// check reviewer names
	if (!ui_->ds_reviewer1->isValidSelection()) error_messages.append("Ungültiger Name für Auswerter 1!");
	if (!ui_->ds_reviewer2->isValidSelection()) error_messages.append("Ungültiger Name für Auswerter 2!");
	// check QLineEdit fields for empty text
	if (ui_->le_dna_rna->text().trimmed() == "") error_messages.append("Ungültige DNA/RNA-Nummer!");
	if (ui_->cb_analysis_scope->currentText().trimmed() == "") error_messages.append("Ungültiger Auswerteumfang!");

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

	evaluation_sheet_data_->analysis_scope = ui_->cb_analysis_scope->currentText();
	evaluation_sheet_data_->acmg_requested = ui_->cb_acmg_requested->isChecked();
	evaluation_sheet_data_->acmg_analyzed = ui_->cb_acmg_analyzed->isChecked();
	evaluation_sheet_data_->acmg_noticeable = ui_->cb_acmg_suspicious->isChecked();

	evaluation_sheet_data_->filtered_by_freq_based_dominant = ui_->cb_filter_freq_dom->isChecked();
	evaluation_sheet_data_->filtered_by_freq_based_recessive = ui_->cb_filter_rec->isChecked();
	evaluation_sheet_data_->filtered_by_mito = ui_->cb_filter_mito->isChecked();
	evaluation_sheet_data_->filtered_by_x_chr = ui_->cb_filter_x_chr->isChecked();
	evaluation_sheet_data_->filtered_by_cnv = ui_->cb_filter_cnv->isChecked();
	evaluation_sheet_data_->filtered_by_svs = ui_->cb_filter_svs->isChecked();
	evaluation_sheet_data_->filtered_by_res = ui_->cb_filter_res->isChecked();
	evaluation_sheet_data_->filtered_by_mosaic = ui_->cb_filter_mosaics->isChecked();
	evaluation_sheet_data_->filtered_by_phenotype = ui_->cb_filter_phen->isChecked();
	evaluation_sheet_data_->filtered_by_multisample = ui_->cb_filter_multi->isChecked();
	evaluation_sheet_data_->filtered_by_trio_stringent = ui_->cb_filter_trio_stringent->isChecked();
	evaluation_sheet_data_->filtered_by_trio_relaxed = ui_->cb_filter_trio_relaxed->isChecked();

	emit EvaluationSheetDataUpdated();

}

void EvaluationSheetEditDialog::checkReviewer()
{
	// try to cast to QLineEdit:
	DBSelector* db_selector = qobject_cast<DBSelector*> (sender());
	if (db_selector == NULL) return;

	// check if reviewer name is valid
	db_selector->showVisuallyIfValid(true);
}

void EvaluationSheetEditDialog::checkNonEmpty()
{
	// try to cast to QLineEdit:
	QLineEdit* line_edit = qobject_cast<QLineEdit*> (sender());
	if (line_edit == NULL) return;

	// set background to red if text is empty
	line_edit->setStyleSheet(line_edit->text().trimmed().isEmpty() ? "QLineEdit {border: 2px solid red;}": "");
}

void EvaluationSheetEditDialog::initReviewerNames()
{
	users_ = db_.createTable("users", "SELECT id, name FROM user ORDER BY name ASC");
	ui_->ds_reviewer1->fill(users_);
	ui_->ds_reviewer2->fill(users_);
}
