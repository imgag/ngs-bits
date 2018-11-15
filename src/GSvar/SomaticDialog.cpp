#include "SomaticDialog.h"
#include <QMessageBox>

SomaticDialog::SomaticDialog(QWidget* parent)
	: QDialog(parent)
	, ui_()
	, db_()
	, samples_()
	, steps_(SingleSampleAnalysisDialog::loadSteps("analysis_steps_somatic"))
{
	ui_.setupUi(this);
	SingleSampleAnalysisDialog::addStepsToParameters(steps_, qobject_cast<QFormLayout*>(ui_.param_group->layout()));
}

void SomaticDialog::setSamples(QList<AnalysisJobSample> samples)
{
	foreach(AnalysisJobSample sample, samples)
	{
		addSample(sample.info, sample.name);
	}
	updateSampleTable();
	updateStartButton();
}

void SomaticDialog::setCustomArguments(const QString& args)
{
	ui_.custom_args->setText(args);
}


QList<AnalysisJobSample> SomaticDialog::samples() const
{
	return SingleSampleAnalysisDialog::samples(samples_);
}

QStringList SomaticDialog::arguments() const
{
	return SingleSampleAnalysisDialog::arguments(this);
}

bool SomaticDialog::highPriority() const
{
	return ui_.high_priority->isChecked();
}

void SomaticDialog::on_add_samples_clicked(bool)
{
	//clear old data
	samples_.clear();
	updateSampleTable();

	//add samples
	try
	{
		addSample("tumor");
		updateSampleTable();
		addSample("normal");
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

void SomaticDialog::updateStartButton()
{
	ui_.start_button->setEnabled(samples_.count()==2);
}

void SomaticDialog::addSample(QString status, QString sample)
{
	SingleSampleAnalysisDialog::addSample(db_, status, samples_, sample);
}

void SomaticDialog::updateSampleTable()
{
	SingleSampleAnalysisDialog::updateSampleTable(samples_, ui_.samples_table);
}
