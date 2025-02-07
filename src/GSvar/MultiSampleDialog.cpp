#include "MultiSampleDialog.h"
#include <QMessageBox>

MultiSampleDialog::MultiSampleDialog(QWidget *parent)
	: QDialog(parent)
	, ui_()
	, db_()
	, samples_()
	, steps_(SingleSampleAnalysisDialog::loadSteps("analysis_steps_multi_sample"))
	, steps_lr_(SingleSampleAnalysisDialog::loadSteps("analysis_steps_multi_sample_lr"))
{
	ui_.setupUi(this);
	SingleSampleAnalysisDialog::initTable(ui_.samples_table);
	SingleSampleAnalysisDialog::addStepsToParameters(steps_, qobject_cast<QFormLayout*>(ui_.param_group->layout()));

	connect(ui_.annotate_only, SIGNAL(stateChanged(int)), this, SLOT(annotate_only_state_changed()));
}

void MultiSampleDialog::setSamples(QList<AnalysisJobSample> samples)
{
	foreach(AnalysisJobSample sample, samples)
	{
		addSample(sample.info, sample.name);
	}

	checkLongread();
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
	checkLongread();
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
	checkLongread();
	updateSampleTable();
	updateStartButton();
}

void MultiSampleDialog::updateStartButton()
{
	ui_.start_button->setEnabled(samples_.count()>=2);
}

void MultiSampleDialog::annotate_only_state_changed()
{
	// get all step check boxes
    QList<QCheckBox*> step_boxes = findChildren<QCheckBox*>(QRegularExpression("^step_"));

	if(ui_.annotate_only->isChecked())
	{
		foreach (QCheckBox* step, step_boxes)
		{
			// deactivate mapping/db import  check box
			if ((step->objectName() == "step_ma") || (step->objectName() == "step_db"))
			{
				step->setChecked(false);
			}

			// activate vc, cn and sv by default
			if ((step->objectName() == "step_vc") || (step->objectName() == "step_cn") || (step->objectName() == "step_sv"))
			{
				step->setChecked(true);
			}
		}
	}
}

void MultiSampleDialog::addSample(QString status, QString sample)
{
	try
	{
		QString analysis_type;
		SingleSampleAnalysisDialog::addSample(db_, status, samples_, analysis_type, sample);
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

void MultiSampleDialog::checkLongread()
{
	if (samples_.size() > 0)
	{
		//check if all samples are either long-read or short-read
		QSet<bool> is_longread_sample;
		foreach (const SampleDetails& sample, samples_)
		{
			is_longread_sample.insert(db_.getProcessedSampleData(db_.processedSampleId(sample.name)).processing_system_type == "lrGS");
		}
		if (is_longread_sample.size() > 1)
		{
			QMessageBox::warning(this, "Error in sample selection", "You cannot mix short-read and long-read samples in multi analysis!");
			samples_.clear();
		}
		else
		{
			//remove old step entries
			QList<QWidget*> old_steps;
			foreach (QWidget* widget, ui_.param_group->findChildren<QWidget *>(QString(), Qt::FindDirectChildrenOnly))
			{
				if (widget->objectName().startsWith("step_")) old_steps.append(widget);
				if (widget->objectName().isEmpty()) old_steps.append(widget);
			}
			qDeleteAll(old_steps);
			//update steps
			if (is_longread_sample.values().first())
			{
				SingleSampleAnalysisDialog::addStepsToParameters(steps_lr_, qobject_cast<QFormLayout*>(ui_.param_group->layout()));
				ui_.annotate_only->setEnabled(false);
				ui_.annotate_only->setChecked(false);
			}
			else
			{
				SingleSampleAnalysisDialog::addStepsToParameters(steps_, qobject_cast<QFormLayout*>(ui_.param_group->layout()));
				ui_.annotate_only->setEnabled(true);
			}

		}
	}
}
