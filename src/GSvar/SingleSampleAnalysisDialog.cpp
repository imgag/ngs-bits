#include "SingleSampleAnalysisDialog.h"
#include <QInputDialog>
#include <QMessageBox>
#include <Settings.h>
#include <QPlainTextEdit>
#include <ProcessedSampleSelector.h>
#include "GUIHelper.h"
#include "ProcessedSampleWidget.h"
#include "GlobalServiceProvider.h"

SingleSampleAnalysisDialog::SingleSampleAnalysisDialog(QWidget *parent)
	: QDialog(parent)
	, ui_()
	, db_()
	, samples_()
{
	ui_.setupUi(this);
	initTable(ui_.samples_table);

	connect(ui_.annotate_only, SIGNAL(stateChanged(int)), this, SLOT(annotate_only_state_changed()));
}

void SingleSampleAnalysisDialog::setAnalysisSteps()
{
	// load and display correct analysis steps
	if (steps_.size() == 0 && samples_.size() > 0)
	{
		if (analysis_type_ == "cfDNA")
		{
			steps_ = loadSteps("analysis_steps_cfdna");
			ui_.annotate_only->setEnabled(true);
			ui_.l_annotation_only->setEnabled(true);
		}
		else if (analysis_type_ == "RNA")
		{
			steps_ = loadSteps("analysis_steps_single_sample_rna");
			ui_.annotate_only->setEnabled(false);
			ui_.l_annotation_only->setEnabled(false);
			ui_.annotate_only->setChecked(false);
		}
		else if (analysis_type_.startsWith("DNA"))
		{
			//check if longread
			QSet<QString> sys_types;
			foreach (const SampleDetails& sample_details, samples_) sys_types.insert(db_.getProcessingSystemData(db_.processingSystemId(sample_details.system)).type);

			if(sys_types.contains("lrGS"))
			{
                if(sys_types.size() > 1) THROW(ArgumentException, "Error: Multiple processing types selected (" + sys_types.values().join(", ") + "). Cannot start analysis!");
				steps_ = loadSteps("analysis_steps_single_sample_lr");
				ui_.annotate_only->setEnabled(false);
				ui_.l_annotation_only->setEnabled(false);
				ui_.annotate_only->setChecked(false);
			}
			else
			{
				steps_ = loadSteps("analysis_steps_single_sample");
				ui_.annotate_only->setEnabled(true);
				ui_.l_annotation_only->setEnabled(true);
			}

		}
		else
		{
			THROW(NotImplementedException, "Invalid analysis type '" + analysis_type_ + "'!");
		}

		// set correct steps
		addStepsToParameters(steps_, qobject_cast<QFormLayout*>(ui_.param_group->layout()));
	}

	updateSampleTable();
	updateStartButton();
}

void SingleSampleAnalysisDialog::setSamples(QList<AnalysisJobSample> samples)
{
	// add samples
	foreach(AnalysisJobSample sample, samples)
	{
		addSample(sample.info, sample.name);
	}
	setAnalysisSteps();
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

QString SingleSampleAnalysisDialog::addSample(NGSD& db, QString status, QList<SampleDetails>& samples, QString& analysis_type, QString ps_name, bool throw_if_bam_missing, bool force_showing_dialog)
{
	status = status.trimmed();

	//get sample name if unset
	if (force_showing_dialog || ps_name.isEmpty())
	{
		ProcessedSampleSelector dlg(GUIHelper::mainWindow(), false);
		dlg.setLabel(status.isEmpty() ? "Processed sample:" : status + ":");
		dlg.setSelection(ps_name);
		if (dlg.exec())
		{
			ps_name = dlg.processedSampleName();
		}
		else
		{
			//If caneceled return empty
			return "";
		}
	}
	if (ps_name.isEmpty()) return "";

	//check NGSD data
	QString ps_id = db.processedSampleId(ps_name);

	//check if sample fits to the selected analysis type
	QString sample_type = db.getSampleData(db.sampleId(ps_name)).type;
	if (sample_type.startsWith("DNA (")) sample_type = "DNA"; //convert "DNA (amplicon)" and "DNA (native)" to "DNA"
	if (analysis_type.isEmpty())
	{
		//set analysis type based on the first sample
		analysis_type = sample_type;
	}
	else
	{
		//check if sample have the same type as the widget
		if (analysis_type != sample_type)
		{
			THROW(ArgumentException, "Sample " + ps_name + " doesn't match previously determined analysis typ (" + analysis_type + ")!");
		}
	}

	//check if added a shortread sample to a longread sample list or the other way around
	if(samples.size() > 0)
	{
		QSet<QString> sys_types;
		foreach (const SampleDetails& sample_details, samples)
		{
			sys_types.insert(db.getProcessingSystemData(db.processingSystemId(sample_details.system)).type);
		}
		bool sample_table_is_longread = sys_types.contains("lrGS");
		bool new_sample_is_longread = db.getProcessingSystemData(db.processingSystemIdFromProcessedSample(ps_name)).type == "lrGS";
		if (sample_table_is_longread != new_sample_is_longread)
		{
			THROW(ArgumentException, "Cannot queue longread and shortread analysis in one batch!");
		}
	}

	//check BAM file exists
	if (throw_if_bam_missing)
	{
		FileLocation bam_file = GlobalServiceProvider::database().processedSamplePath(ps_id, PathType::BAM);
		if (!bam_file.exists)
		{
			THROW(FileAccessException, "Sample BAM file does not exist: '" + bam_file.filename);
		}
	}

	//add sample
	ProcessedSampleData processed_sample_data = db.getProcessedSampleData(ps_id);
	QString s_id = db.sampleId(ps_name);
	SampleData sample_data = db.getSampleData(s_id);
	samples.append(SampleDetails {ps_name, processed_sample_data.processing_system, status, processed_sample_data.quality, processed_sample_data.gender, sample_data.disease_group, sample_data.disease_status});

	return ps_id;
}

void SingleSampleAnalysisDialog::initTable(QTableWidget* samples_table)
{
	samples_table->clear();
	samples_table->setColumnCount(7);
	samples_table->setHorizontalHeaderItem(0, new QTableWidgetItem("processed sample"));
	samples_table->setHorizontalHeaderItem(1, new QTableWidgetItem("status"));
	samples_table->setHorizontalHeaderItem(2, new QTableWidgetItem("quality"));
	samples_table->setHorizontalHeaderItem(3, new QTableWidgetItem("processing system"));
	samples_table->setHorizontalHeaderItem(4, new QTableWidgetItem("gender"));
	samples_table->setHorizontalHeaderItem(5, new QTableWidgetItem("disease group"));
	samples_table->setHorizontalHeaderItem(6, new QTableWidgetItem("disease status"));
}

void SingleSampleAnalysisDialog::updateSampleTable(const QList<SampleDetails>& samples, QTableWidget* samples_table)
{
	samples_table->clearContents();
	samples_table->setRowCount(samples.count());
	for (int i=0; i<samples.count(); ++i)
	{
		samples_table->setItem(i, 0, new QTableWidgetItem(samples[i].name));
		samples_table->setItem(i, 1, new QTableWidgetItem(samples[i].status));
		QLabel* quality_label = new QLabel();
		quality_label->setAlignment(Qt::AlignLeft|Qt::AlignVCenter);
		quality_label->setMargin(1);
		ProcessedSampleWidget::styleQualityLabel(quality_label, samples[i].quality);
		samples_table->setCellWidget(i, 2, quality_label);
		samples_table->setItem(i, 3, new QTableWidgetItem(samples[i].system));
		samples_table->setItem(i, 4, new QTableWidgetItem(samples[i].gender));
		samples_table->setItem(i, 5, new QTableWidgetItem(samples[i].disease_group));
		samples_table->setItem(i, 6, new QTableWidgetItem(samples[i].disease_status));
	}

	GUIHelper::resizeTableCellWidths(samples_table);
	GUIHelper::resizeTableCellHeightsToFirst(samples_table);
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
    QList<QCheckBox*> step_boxes = widget->findChildren<QCheckBox*>(QRegularExpression("^step_"));
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
    QList<QCheckBox*> step_boxes = findChildren<QCheckBox*>(QRegularExpression("^step_"));

	if(ui_.annotate_only->isChecked())
	{
		foreach (QCheckBox* step, step_boxes)
		{
			// deactivate mapping/db import  check box
			if (step->objectName()=="step_ma" || step->objectName()=="step_re" || step->objectName()=="step_db")
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
		addSample(db_, status, samples_, analysis_type_, sample, false);
	}
	catch(const Exception& e)
	{
		QMessageBox::warning(this, "Error adding sample", e.message());
		samples_.clear();
	}

	setAnalysisSteps();
}

void SingleSampleAnalysisDialog::updateSampleTable()
{
	updateSampleTable(samples_, ui_.samples_table);
}
