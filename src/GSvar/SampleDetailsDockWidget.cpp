#include "SampleDetailsDockWidget.h"
#include "SingleSampleAnalysisDialog.h"
#include "DiagnosticStatusWidget.h"
#include "GUIHelper.h"
#include "BasicStatistics.h"
#include "Settings.h"
#include <QMenu>
#include <QStringList>
#include <QFileInfo>

SampleDetailsDockWidget::SampleDetailsDockWidget(QWidget *parent)
	: QDockWidget(parent)
	, ui_()
	, processed_sample_name_()
{
	ui_.setupUi(this);

	//diagnostic status button
	connect(ui_.diag_status_button, SIGNAL(clicked(bool)), this, SLOT(editDiagnosticStatus()));
	connect(ui_.restart_button, SIGNAL(clicked(bool)), this, SLOT(reanalyze()));

	if (Settings::boolean("NGSD_enabled", true))
	{
		//setup quality button
		QMenu* menu = new QMenu();
		QStringList quality = NGSD().getEnum("processed_sample", "quality");
		foreach(QString q, quality)
		{
			menu->addAction(q, this, SLOT(setQuality()));
		}
		ui_.quality_button->setMenu(menu);
	}

	clear();
}


void SampleDetailsDockWidget::reanalyze()
{
	SingleSampleAnalysisDialog dlg(this);

	dlg.setSamples(QList<AnalysisJobSample>() << AnalysisJobSample {processed_sample_name_, ""});
	if (dlg.exec()==QDialog::Accepted)
	{
		NGSD().queueAnalysis("single sample", dlg.highPriority(), dlg.arguments(), dlg.samples());
	}
}

void SampleDetailsDockWidget::editDiagnosticStatus()
{
	NGSD db;
	QString processed_sample_id = db.processedSampleId(processed_sample_name_);

	DiagnosticStatusWidget* widget = new DiagnosticStatusWidget(this);
	widget->setStatus(db.getDiagnosticStatus(processed_sample_id));
	auto dlg = GUIHelper::showWidgetAsDialog(widget, "Diagnostic status of " + processed_sample_name_, true);
	if (dlg->result()!=QDialog::Accepted) return;

	db.setDiagnosticStatus(processed_sample_id, widget->status());

	refresh(processed_sample_name_);
}

void SampleDetailsDockWidget::setQuality()
{
	NGSD db;
	QString quality = qobject_cast<QAction*>(sender())->text();
	QString processed_sample_id = db.processedSampleId(processed_sample_name_);
	db.setProcessedSampleQuality(processed_sample_id, quality);

	refresh(processed_sample_name_);
}

void SampleDetailsDockWidget::refresh(QString processed_sample_name)
{
	processed_sample_name_ = processed_sample_name;
	ui_.name->setText(processed_sample_name_);

	if (!Settings::boolean("NGSD_enabled", true)) return;
	NGSD db;

	//sample data from NGSD
	try
	{
		SampleData sample_data = db.getSampleData(db.sampleId(processed_sample_name_));
		ui_.name_ext->setText(sample_data.name_external);
		ui_.tumor_ffpe->setText(QString(sample_data.is_tumor ? "yes" : "no") + " / " + (sample_data.is_ffpe ? "yes" : "no"));
		ui_.disease_group->setText(sample_data.disease_group);
		ui_.disease_status->setText(sample_data.disease_status);
	}
	catch(...)
	{
	}

	//processed sample data from NGSD
	try
	{
		//QC data
		QString processed_sample_id = db.processedSampleId(processed_sample_name_);
		ProcessedSampleData processed_sample_data = db.getProcessedSampleData(processed_sample_id);
		QCCollection qc = db.getQCData(processed_sample_id);
		ui_.qc_reads->setText(qc.value("QC:2000005", true).toString(0) + " (length: " + qc.value("QC:2000006", true).toString(0) + ")");

		statisticsLabel(ui_.qc_avg_depth, "QC:2000025", qc, true, false);
		statisticsLabel(ui_.qc_perc_20x, "QC:2000027", qc, true, false, 2, "%");
		statisticsLabel(ui_.qc_insert_size,"QC:2000023", qc, true, true);
		statisticsLabel(ui_.qc_vars,"QC:2000013", qc, true, true, 0);
		statisticsLabel(ui_.qc_vars_known,"QC:2000014", qc, true, false, 2, "%");
		statisticsLabel(ui_.qc_af_dev,"QC:2000051", qc, false, true, 4, "%");

		ui_.qc_kasp->setText(qc.value("kasp").asString());
		QString quality = processed_sample_data.quality;
		if (quality=="bad") quality = "<font color='red'>"+quality+"</font>";
		if (quality=="medium") quality = "<font color='orange'>"+quality+"</font>";
		ui_.qc_quality->setText(quality);
		ui_.comment->setText(processed_sample_data.comments);
		ui_.system->setText(processed_sample_data.processing_system);
		ui_.gender->setText(processed_sample_data.gender);

		//diagnostic status
		DiagnosticStatusData diag_status = db.getDiagnosticStatus(processed_sample_id);
		ui_.diag_status->setText(diag_status.dagnostic_status);
		ui_.diag_outcome->setText(diag_status.outcome);
		ui_.diag_genes->setText(diag_status.genes_causal);
		ui_.diag_inheritance->setText(diag_status.inheritance_mode);
		ui_.diag_comments->setText(diag_status.comments);

		//last analysis (BAM file date)
		QDateTime last_modified = QFileInfo(db.processedSamplePath(processed_sample_id, NGSD::BAM)).lastModified();
		QString date_text = last_modified.toString("yyyy-MM-dd");
		if (last_modified < QDateTime::currentDateTime().addMonths(-6))
		{
			date_text = " <font color='red'>" + date_text + "</font>";
		}
		ui_.date_bam->setText(date_text);

		ui_.quality_button->setEnabled(true);
		ui_.diag_status_button->setEnabled(true);
		ui_.restart_button->setEnabled(true);

	}
	catch(...)
	{
		ui_.quality_button->setEnabled(false);
		ui_.diag_status_button->setEnabled(false);
		ui_.restart_button->setEnabled(false);
	}
}

void SampleDetailsDockWidget::clear()
{
	//labels
	ui_.name->clear();
	ui_.name_ext->clear();
	ui_.tumor_ffpe->clear();
	ui_.disease_group->clear();
	ui_.disease_status->clear();
	ui_.qc_reads->clear();
	ui_.qc_kasp->clear();
	ui_.qc_quality->clear();
	ui_.qc_insert_size->clear();
	ui_.qc_avg_depth->clear();
	ui_.qc_perc_20x->clear();
	ui_.qc_vars->clear();
	ui_.qc_vars_known->clear();
	ui_.qc_af_dev->clear();
	ui_.comment->clear();
	ui_.system->clear();
	ui_.gender->clear();
	ui_.diag_status->clear();
	ui_.diag_outcome->clear();
	ui_.diag_genes->clear();
	ui_.diag_inheritance->clear();
	ui_.diag_comments->clear();
	ui_.date_bam->clear();

	//buttons
	ui_.restart_button->setEnabled(false);
	ui_.diag_status_button->setEnabled(false);
	ui_.quality_button->setEnabled(false);
}

void SampleDetailsDockWidget::statisticsLabel(QLabel* label, QString accession, const QCCollection& qc, bool label_outlier_low, bool label_outlier_high, int decimal_places, QString suffix)
{
	try
	{
		//get QC value
		QString value_str = qc.value(accession, true).toString();
		label->setText(value_str + suffix);

		//get QC value statistics
		NGSD db;
		QVector<double> values = db.getQCValues(accession, db.processedSampleId(processed_sample_name_));
		std::sort(values.begin(), values.end());
		double median = BasicStatistics::median(values, false);
		double mad = 1.428 * BasicStatistics::mad(values, median);
		label->setToolTip("mean: " + QString::number(median, 'f', decimal_places) + "<br>stdev: " + QString::number(mad, 'f', decimal_places));

		//mark QC value red if it is an outlier
		bool ok = false;
		double value = value_str.toDouble(&ok);
		if (!ok || (label_outlier_low && value < median-2*mad) || (label_outlier_high && value > median+2*mad))
		{
			label->setText("<font color='red'>" + value_str + suffix + "</font>");
		}
	}
	catch (ArgumentException e) //in case the QC value does not exist in the DB
	{
		label->setText("<font color='red'>n/a</font>");
		label->setToolTip("NGSD error: " + e.message());
	}
	catch (StatisticsException e) //in case the QC values statistics cannot be calculated
	{
		label->setToolTip("Statistics error: " + e.message());
	}
}
