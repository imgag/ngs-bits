#include "TrioDialog.h"
#include <QMessageBox>

TrioDialog::TrioDialog(QWidget* parent)
	: QDialog(parent)
	, ui_()
	, db_()
	, samples_()
	, steps_(SingleSampleAnalysisDialog::loadSteps("analysis_steps_trio"))
{
	ui_.setupUi(this);
	SingleSampleAnalysisDialog::addStepsToParameters(steps_, qobject_cast<QFormLayout*>(ui_.param_group->layout()));
}

void TrioDialog::setSamples(QList<AnalysisJobSample> samples)
{
	foreach(AnalysisJobSample sample, samples)
	{
		addSample(sample.info, sample.name);
	}
	updateSampleTable();
	updateStartButton();
}

QList<AnalysisJobSample> TrioDialog::samples() const
{
	return SingleSampleAnalysisDialog::samples(samples_);
}

QStringList TrioDialog::arguments() const
{
	return SingleSampleAnalysisDialog::arguments(this);
}

bool TrioDialog::highPriority() const
{
	return ui_.high_priority->isChecked();
}

void TrioDialog::on_add_samples_clicked(bool)
{
	//clear old data
	samples_.clear();
	updateSampleTable();

	//add samples
	try
	{
		addSample("child");
		updateSampleTable();
		addSample("father");
		updateSampleTable();
		addSample("mother");
	}
	catch(const Exception& e)
	{
		QMessageBox::warning(this, "Error adding sample", e.message());
		samples_.clear();
	}

	//update table/button
	updateSampleTable();
	updateStartButton();
}

void TrioDialog::updateStartButton()
{
	ui_.start_button->setEnabled(samples_.count()==3);
}

void TrioDialog::addSample(QString status, QString sample)
{
	SingleSampleAnalysisDialog::addSample(db_, status, samples_, sample);
}

void TrioDialog::updateSampleTable()
{
	SingleSampleAnalysisDialog::updateSampleTable(samples_, ui_.samples_table);
}
