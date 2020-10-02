#include "SingleSampleAnalysisDialog.h"
#include <QInputDialog>
#include <QMessageBox>
#include <Settings.h>
#include <QPlainTextEdit>
#include <ProcessedSampleSelector.h>
#include "GUIHelper.h"
#include "ProcessedSampleWidget.h"

SingleSampleAnalysisDialog::SingleSampleAnalysisDialog(QWidget *parent)
	: QDialog(parent)
	, ui_()
	, db_()
	, samples_()
	, steps_(loadSteps("analysis_steps_single_sample"))
{
	ui_.setupUi(this);
	addStepsToParameters(steps_, qobject_cast<QFormLayout*>(ui_.param_group->layout()));

	connect(ui_.annotate_only, SIGNAL(stateChanged(int)), this, SLOT(annotate_only_state_changed()));
}

void SingleSampleAnalysisDialog::setSamples(QList<AnalysisJobSample> samples)
{
	foreach(AnalysisJobSample sample, samples)
	{
		addSample(sample.info, sample.name);
	}
	updateSampleTable();
	updateStartButton();
}

QList<AnalysisJobSample> SingleSampleAnalysisDialog::samples() const
{
	return samples(samples_);
}

QStringList SingleSampleAnalysisDialog::arguments() const
{
	return arguments(this);
}

bool SingleSampleAnalysisDialog::highPriority() const
{
	return ui_.high_priority->isChecked();
}

QString SingleSampleAnalysisDialog::addSample(NGSD& db, QString status, QList<SampleDetails>& samples, QString sample, bool throw_if_bam_missing, bool force_showing_dialog)
{
	status = status.trimmed();

	//get sample name if unset
	if (force_showing_dialog || sample.isEmpty())
	{
		ProcessedSampleSelector dlg(QApplication::activeWindow(), false);
		dlg.setLabel(status.isEmpty() ? "Processed sample:" : status + ":");
		dlg.setSelection(sample);
		if (dlg.exec())
		{
			sample = dlg.processedSampleName();
		}
	}
	if (sample.isEmpty()) return "";

	//check NGSD data
	QString ps_id = db.processedSampleId(sample);

	//check BAM file exists
	if (throw_if_bam_missing)
	{
		QString bam = db.processedSamplePath(ps_id, PathType::BAM);
		if (!QFile::exists(bam))
		{
			THROW(FileAccessException, "Sample BAM file does not exist: '" + bam);
		}
	}

	//add sample
	ProcessedSampleData processed_sample_data = db.getProcessedSampleData(ps_id);
	samples.append(SampleDetails {sample, processed_sample_data.processing_system, status, processed_sample_data.quality, processed_sample_data.gender});

	return ps_id;
}

void SingleSampleAnalysisDialog::updateSampleTable(const QList<SampleDetails>& samples, QTableWidget* samples_table)
{
	samples_table->clearContents();
	samples_table->setRowCount(samples.count());
	for (int i=0; i<samples.count(); ++i)
	{
		samples_table->setItem(i, 0, new QTableWidgetItem(samples[i].name));
		samples_table->setItem(i, 1, new QTableWidgetItem(samples[i].system));
		samples_table->setItem(i, 2, new QTableWidgetItem(samples[i].status));
		QLabel* quality_label = new QLabel();
		quality_label->setAlignment(Qt::AlignLeft|Qt::AlignVCenter);
		quality_label->setMargin(1);
		ProcessedSampleWidget::styleQualityLabel(quality_label, samples[i].quality);
		samples_table->setCellWidget(i, 3, quality_label);
		samples_table->setItem(i, 4, new QTableWidgetItem(samples[i].gender));
	}

	GUIHelper::resizeTableCells(samples_table);
}

QList<AnalysisJobSample> SingleSampleAnalysisDialog::samples(const QList<SampleDetails>& samples)
{
	QList<AnalysisJobSample> output;

	foreach(const SampleDetails& info, samples)
	{
		output << AnalysisJobSample { info.name, info.status };
	}

	return output;
}

QList<AnalysisStep> SingleSampleAnalysisDialog::loadSteps(QString ini_name)
{
	QList<AnalysisStep> output;

	QStringList tmp = Settings::stringList(ini_name);
	foreach(QString step, tmp)
	{
		int index = step.indexOf("-");
		output << AnalysisStep { step.left(index).trimmed(), step.mid(index+1).trimmed() };
	}

	return output;
}

void SingleSampleAnalysisDialog::addStepsToParameters(QList<AnalysisStep> steps, QFormLayout* layout)
{
	if (steps.count()<2) return;

	bool first_step = true;
	foreach(AnalysisStep step, steps)
	{
		QLabel* label = nullptr;
		if (first_step)
		{
			label = new QLabel("analysis steps:");
			first_step = false;
		}

		QCheckBox* box = new QCheckBox(step.description);
		box->setObjectName("step_" + step.name);
		box->setChecked(true);
		layout->addRow(label, box);
	}
}

QStringList SingleSampleAnalysisDialog::arguments(const QWidget* widget)
{
	QStringList output;

	QLineEdit* custom = widget->findChild<QLineEdit*>("custom_args");
	if (custom!=nullptr && !custom->text().isEmpty())
	{
		output << custom->text();
	}

	QCheckBox* anno_only = widget->findChild<QCheckBox*>("annotate_only");
	if (anno_only!=nullptr && anno_only->isChecked()) output << "-annotation_only";

	QStringList steps;
	QList<QCheckBox*> step_boxes = widget->findChildren<QCheckBox*>(QRegExp("^step_"));
	if (step_boxes.count()>0)
	{
		foreach(QCheckBox* box, step_boxes)
		{
			if (box->isChecked())
			{
				steps << box->objectName().mid(5);
			}
		}
		// do not set "-steps" parameter if no steps are checked
		if (steps.size() > 0) output << "-steps " + steps.join(",");
	}

	return output;
}

void SingleSampleAnalysisDialog::on_add_sample_clicked(bool)
{
	addSample("");
	updateSampleTable();
	updateStartButton();
}

void SingleSampleAnalysisDialog::on_add_batch_clicked(bool)
{
	//edit
	QPlainTextEdit* edit = new QPlainTextEdit(this);
	auto dlg = GUIHelper::createDialog(edit, "Enter processed samples (one per line).", "", true);
	if (dlg->exec()==QDialog::Accepted)
	{
		QStringList samples = edit->toPlainText().split('\n');
		foreach(QString sample, samples)
		{
			sample = sample.trimmed();
			if (sample.contains("\t")) sample = sample.split("\t").first();
			if (sample.isEmpty() || sample[0]=='#') continue;
			addSample("", sample);
		}

		updateSampleTable();
		updateStartButton();
	}
}

void SingleSampleAnalysisDialog::on_clear_clicked(bool)
{
	samples_.clear();
	updateSampleTable();
	updateStartButton();
}

void SingleSampleAnalysisDialog::updateStartButton()
{
	ui_.start_button->setEnabled(samples_.count()>=1);
}

void SingleSampleAnalysisDialog::annotate_only_state_changed()
{
	// get all step check boxes
	QList<QCheckBox*> step_boxes = this->findChildren<QCheckBox*>(QRegExp("^step_"));

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

void SingleSampleAnalysisDialog::addSample(QString status, QString sample)
{
	try
	{
		addSample(db_, status, samples_, sample, false);
	}
	catch(const Exception& e)
	{
		QMessageBox::warning(this, "Error adding sample", e.message());
		samples_.clear();
	}
}

void SingleSampleAnalysisDialog::updateSampleTable()
{
	updateSampleTable(samples_, ui_.samples_table);
}
