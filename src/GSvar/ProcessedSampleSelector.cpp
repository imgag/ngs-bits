#include "ProcessedSampleSelector.h"
#include <QInputDialog>
#include <QMessageBox>

ProcessedSampleSelector::ProcessedSampleSelector(QWidget* parent, bool include_merged_samples)
	: QDialog(parent)
	, ui_()
{
	ui_.setupUi(this);
	ui_.show_merged->setChecked(include_merged_samples);
	ui_.search_multi->setVisible(false);

	connect(ui_.show_merged, SIGNAL(stateChanged(int)), this, SLOT(initAutoCompletion()));
	connect(ui_.open_by_id, SIGNAL(clicked(bool)), this, SLOT(openById()));

	initAutoCompletion();
}

void ProcessedSampleSelector::setLabel(QString label)
{
	ui_.label->setText(label);
}

void ProcessedSampleSelector::setSelection(QString processed_sample)
{
	ui_.processed_sample->setText(processed_sample);
}

bool ProcessedSampleSelector::isValidSelection() const
{
	return  ui_.processed_sample->isValidSelection();
}

void ProcessedSampleSelector::initAutoCompletion()
{
	QString query = "SELECT ps.id, CONCAT(s.name,'_',LPAD(ps.process_id,2,'0')) FROM sample s, processed_sample ps WHERE ps.sample_id=s.id";
	if (!ui_.show_merged->isChecked()) query += " AND ps.id NOT IN (SELECT processed_sample_id FROM merged_processed_samples)";
	ui_.processed_sample->fill(db_.createTable("processed_sample", query));
}

void ProcessedSampleSelector::openById()
{
	QString title = "Select by ID";
	QString ps_id = QInputDialog::getText(this, title , "processed sample NSGD id:").trimmed();
	if (ps_id=="") return;

	NGSD db;
	QString ps = db.processedSampleName(ps_id, false);
	if (ps=="")
	{
		QMessageBox::warning(this, title, "Invalid processed sample ID '" + ps_id + "' given!");
		return;
	}

	ui_.processed_sample->setText(ps);
}

QString ProcessedSampleSelector::processedSampleId() const
{
	return  ui_.processed_sample->getId();
}

QString ProcessedSampleSelector::processedSampleName() const
{
	QString ps_id = processedSampleId();
	if (ps_id=="") return "";

	return db_.processedSampleName(ps_id);
}

void ProcessedSampleSelector::showSearchMulti(bool checked)
{
	ui_.search_multi->setVisible(true);
	ui_.search_multi->setChecked(checked);
}

bool ProcessedSampleSelector::searchMulti() const
{
	return ui_.search_multi->isChecked();
}
