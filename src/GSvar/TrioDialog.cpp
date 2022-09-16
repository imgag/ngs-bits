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
	SingleSampleAnalysisDialog::initTable(ui_.samples_table);
	SingleSampleAnalysisDialog::addStepsToParameters(steps_, qobject_cast<QFormLayout*>(ui_.param_group->layout()));

	connect(ui_.annotate_only, SIGNAL(stateChanged(int)), this, SLOT(annotate_only_state_changed()));
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
		QString c_id = addSample("child");
		QString c_sys_id = db_.getValue("SELECT processing_system_id FROM processed_sample WHERE id='" + c_id + "'").toString();
		QString c_s_id = db_.getValue("SELECT sample_id FROM processed_sample WHERE id='" + c_id + "'").toString();
		updateSampleTable();

		//try to find father (male, parent relation, same processing system)
		QStringList f_ps_ids = db_.getValues("SELECT ps.id FROM processed_sample ps, sample s, sample_relations sr WHERE ps.sample_id=s.id AND s.id=sr.sample1_id AND sr.relation='parent-child' AND s.gender='male' AND ps.processing_system_id='" + c_sys_id + "' AND sr.sample2_id='" + c_s_id + "'");
		if (f_ps_ids.count()==1)
		{
			addSample("father",  db_.processedSampleName(f_ps_ids[0]), true);
		}
		else
		{
			addSample("father");
		}
		updateSampleTable();

		//try to find mother (female, parent relation, same processing system)
		QStringList m_ps_ids = db_.getValues("SELECT ps.id FROM processed_sample ps, sample s, sample_relations sr WHERE ps.sample_id=s.id AND s.id=sr.sample1_id AND sr.relation='parent-child' AND s.gender='female' AND ps.processing_system_id='" + c_sys_id + "' AND sr.sample2_id='" + c_s_id + "'");
		if (m_ps_ids.count()==1)
		{
			addSample("mother", db_.processedSampleName(m_ps_ids[0]), true);
		}
		else
		{
			addSample("mother");
		}
	}
	catch(const AbortByUserException& e)
	{
		samples_.clear();
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

void TrioDialog::annotate_only_state_changed()
{
	// get all step check boxes
	QList<QCheckBox*> step_boxes = findChildren<QCheckBox*>(QRegExp("^step_"));

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

QString TrioDialog::addSample(QString status, QString sample, bool force_showing_dialog)
{
	QString analysis_type;
	QString ps_id = SingleSampleAnalysisDialog::addSample(db_, status, samples_, analysis_type, sample, false, force_showing_dialog);
	if (ps_id.isEmpty()) THROW(AbortByUserException, "");
	return ps_id;
}

void TrioDialog::updateSampleTable()
{
	SingleSampleAnalysisDialog::updateSampleTable(samples_, ui_.samples_table);
}
