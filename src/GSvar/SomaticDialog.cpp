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
	SingleSampleAnalysisDialog::initTable(ui_.samples_table);
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

bool SomaticDialog::useDragen() const
{
	return ui_.use_dragen->isChecked();
}

void SomaticDialog::on_add_samples_clicked(bool)
{
	//clear old data
	samples_.clear();
	updateSampleTable();

	//add samples
	try
	{
		QString t_id = addSample("tumor");
		updateSampleTable();
		addSample("normal", db_.normalSample(t_id), true);
	}
	catch(const AbortByUserException& e)
	{
		if(!( samples_.count() == 1 && samples_[0].status == "tumor" )) //user shall be able to queue tumor only
		{
			samples_.clear();
		}
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
	//tumor normal analysis
	bool tumor_normal = samples_.count()==2;

	ui_.start_button->setEnabled(tumor_normal);
	ui_.use_dragen->setEnabled(tumor_normal);
	if (! tumor_normal )ui_.use_dragen->setChecked(false);
	ui_.l_use_dragen->setEnabled(tumor_normal);

	foreach(QObject* child, ui_.param_group->children())
	{
		if (child->objectName() == "step_msi")
		{
			QCheckBox* step = qobject_cast<QCheckBox*>(child);
			step->setEnabled(tumor_normal);
			step->setChecked(tumor_normal);
		}
	}

	//tumor only analysis
	if(samples_.count() == 1 && samples_[0].status == "tumor") ui_.start_button->setEnabled(true);
}


QString SomaticDialog::addSample(QString status, QString sample, bool force_showing_dialog)
{
	QString analysis_type;
	QString ps_id = SingleSampleAnalysisDialog::addSample(db_, status, samples_, analysis_type, sample, true, force_showing_dialog);
	if (ps_id.isEmpty()) THROW(AbortByUserException, "");
	return ps_id;
}

void SomaticDialog::updateSampleTable()
{
	SingleSampleAnalysisDialog::updateSampleTable(samples_, ui_.samples_table);
}
