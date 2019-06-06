#include "ProcessedSampleSelector.h"

ProcessedSampleSelector::ProcessedSampleSelector(QWidget* parent, bool include_merged_samples)
	: QDialog(parent)
	, ui_()
{
	ui_.setupUi(this);
	ui_.show_merged->setChecked(include_merged_samples);

	connect(ui_.show_merged, SIGNAL(stateChanged(int)), this, SLOT(initAutoCompletion()));

	initAutoCompletion();
}

void ProcessedSampleSelector::setLabel(QString label)
{
	ui_.label->setText(label);
}

void ProcessedSampleSelector::setSelection(QString procssed_sample)
{
	ui_.processed_sample->setText(procssed_sample);
}

bool ProcessedSampleSelector::isValidSelection() const
{
	return  ui_.processed_sample->isValidSelection();
}

void ProcessedSampleSelector::initAutoCompletion()
{
	QString query = "SELECT ps.id, CONCAT(s.name,'_',LPAD(ps.process_id,2,'0')) FROM sample s, processed_sample ps WHERE ps.sample_id=s.id AND ps.id";
	if (!ui_.show_merged->isChecked()) query += " NOT IN (SELECT processed_sample_id FROM merged_processed_samples)";
	ui_.processed_sample->fill(db_.createTable("processed_sample", query));
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
