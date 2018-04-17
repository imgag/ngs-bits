#include "MultiSampleDialog.h"
#include <QMessageBox>

MultiSampleDialog::MultiSampleDialog(QWidget *parent)
	: QDialog(parent)
	, ui_()
	, db_()
	, samples_()
	, steps_(SingleSampleAnalysisDialog::loadSteps("analysis_steps_multi_sample"))
{
	ui_.setupUi(this);
	SingleSampleAnalysisDialog::addStepsToParameters(steps_, qobject_cast<QFormLayout*>(ui_.param_group->layout()));
}

void MultiSampleDialog::setSamples(QList<AnalysisJobSample> samples)
{
	foreach(AnalysisJobSample sample, samples)
	{
		addSample(sample.info, sample.name);
	}
	updateSampleTable();
	updateStartButton();
}

QList<AnalysisJobSample> MultiSampleDialog::samples() const
{
	return SingleSampleAnalysisDialog::samples(samples_);
}

QStringList MultiSampleDialog::arguments() const
{
	return SingleSampleAnalysisDialog::arguments(this);
}

bool MultiSampleDialog::highPriority() const
{
	return ui_.high_priority->isChecked();
}

void MultiSampleDialog::on_add_affected_clicked(bool)
{
	addSample("affected");
	updateSampleTable();
	updateStartButton();
}

void MultiSampleDialog::on_clear_clicked(bool)
{
	samples_.clear();
	updateSampleTable();
	updateStartButton();
}

void MultiSampleDialog::on_add_control_clicked(bool)
{
	addSample("control");
	updateSampleTable();
	updateStartButton();
}

void MultiSampleDialog::updateStartButton()
{
	ui_.start_button->setEnabled(samples_.count()>=2);
}


void MultiSampleDialog::addSample(QString status, QString sample)
{
	try
	{
		SingleSampleAnalysisDialog::addSample(db_, status, samples_, sample);
	}
	catch(const Exception& e)
	{
		QMessageBox::warning(this, "Error adding sample", e.message());
		samples_.clear();
	}
}

void MultiSampleDialog::updateSampleTable()
{
	SingleSampleAnalysisDialog::updateSampleTable(samples_, ui_.samples_table);
}

