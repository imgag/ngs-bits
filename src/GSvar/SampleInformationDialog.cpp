#include "SampleInformationDialog.h"

#include <QMenu>
#include <QFileInfo>
#include "GUIHelper.h"
#include "BasicStatistics.h"
#include "DiagnosticStatusWidget.h"
#include "SingleSampleAnalysisDialog.h"

SampleInformationDialog::SampleInformationDialog(QWidget* parent, QString processed_sample_name)
	: QDialog(parent)
	, ui_()
	, db_()
	, processed_sample_name_(processed_sample_name)
{
	//set busy cursor
	QApplication::setOverrideCursor(QCursor(Qt::BusyCursor));

	//setup of UI
	ui_.setupUi(this);

	//diagnostic status button
	connect(ui_.diag_status_button, SIGNAL(clicked(bool)), this, SLOT(editDiagnosticStatus()));
	connect(ui_.restart_button, SIGNAL(clicked(bool)), this, SLOT(reanalyze()));

	//setup quality button
	QMenu* menu = new QMenu();
	QStringList quality = db_.getEnum("processed_sample", "quality");
	foreach(QString q, quality)
	{
		menu->addAction(q, this, SLOT(setQuality()));
	}
	ui_.quality_button->setMenu(menu);

	//refresh
	refresh();

	//reset busy cursor
	QApplication::restoreOverrideCursor();
}

void SampleInformationDialog::reanalyze()
{
	SingleSampleAnalysisDialog dlg(this);

	dlg.setSamples(QList<AnalysisJobSample>() << AnalysisJobSample {processed_sample_name_, ""});
	if (dlg.exec()==QDialog::Accepted)
	{
		db_.queueAnalysis("single sample", dlg.highPriority(), dlg.arguments(), dlg.samples());
		refresh();
	}
}

void SampleInformationDialog::editDiagnosticStatus()
{
	QString processed_sample_id = db_.processedSampleId(processed_sample_name_);

	DiagnosticStatusWidget* widget = new DiagnosticStatusWidget(this);
	widget->setStatus(db_.getDiagnosticStatus(processed_sample_id));
	auto dlg = GUIHelper::showWidgetAsDialog(widget, "Diagnostic status of " + processed_sample_name_, true);
	if (dlg->result()!=QDialog::Accepted) return;

	db_.setDiagnosticStatus(processed_sample_id, widget->status());
	refresh();
}

void SampleInformationDialog::setQuality()
{
	QString quality = qobject_cast<QAction*>(sender())->text();
	QString processed_sample_id = db_.processedSampleId(processed_sample_name_);
	db_.setProcessedSampleQuality(processed_sample_id, quality);

	refresh();
}

void SampleInformationDialog::refresh()
{
	//sample data
	ui_.name->setText(processed_sample_name_);
	try
	{
		SampleData sample_data = db_.getSampleData(db_.sampleId(processed_sample_name_));
		ui_.name_ext->setText(sample_data.name_external);
		ui_.tumor_ffpe->setText(QString(sample_data.is_tumor ? "yes" : "no") + " / " + (sample_data.is_ffpe ? "yes" : "no"));
		ui_.disease_group->setText(sample_data.disease_group);
		ui_.disease_status->setText(sample_data.disease_status);
	}
	catch(...)
	{
	}

	//processed sample data
	try
	{
		//QC data
		QString processed_sample_id = db_.processedSampleId(processed_sample_name_);
		ProcessedSampleData processed_sample_data = db_.getProcessedSampleData(processed_sample_id);
		QCCollection qc = db_.getQCData(processed_sample_id);
		ui_.qc_reads->setText(qc.value("QC:2000005", true).toString(0) + " (length: " + qc.value("QC:2000006", true).toString(0) + ")");

		statisticsLabel(ui_.qc_avg_depth, "QC:2000025", qc, true, false);
		statisticsLabel(ui_.qc_perc_20x, "QC:2000027", qc, true, false);
		statisticsLabel(ui_.qc_insert_size,"QC:2000023", qc, true, true);
		statisticsLabel(ui_.qc_af_dev,"QC:2000051", qc, false, true, 4);

		ui_.qc_kasp->setText(qc.value("kasp").asString());
		QString quality = processed_sample_data.quality;
		if (quality=="bad") quality = "<font color='red'>"+quality+"</font>";
		if (quality=="medium") quality = "<font color='orange'>"+quality+"</font>";
		ui_.qc_quality->setText(quality);
		ui_.comment->setText(processed_sample_data.comments);
		ui_.system->setText(processed_sample_data.processing_system);

		//diagnostic status
		DiagnosticStatusData diag_status = db_.getDiagnosticStatus(processed_sample_id);
		ui_.diag_status->setText(diag_status.dagnostic_status);
		ui_.diag_outcome->setText(diag_status.outcome);

		//last analysis (BAM file date)
		QDateTime last_modified = QFileInfo(db_.processedSamplePath(processed_sample_id, NGSD::BAM)).lastModified();
		QString date_text = last_modified.toString("yyyy-MM-dd");
		if (last_modified < QDateTime::currentDateTime().addMonths(-6))
		{
			date_text = " <font color='red'>" + date_text + "</font>";
		}
		ui_.date_bam->setText(date_text);

	}
	catch(...)
	{
		ui_.quality_button->setEnabled(false);
		ui_.diag_status_button->setEnabled(false);
	}
}


void SampleInformationDialog::statisticsLabel(QLabel* label, QString accession, const QCCollection& qc, bool label_outlier_low, bool label_outlier_high, int decimal_places)
{
	try
	{
		//get QC value
		QString value_str = qc.value(accession, true).toString();
		label->setText(value_str);

		//get QC value statistics
		QVector<double> values = db_.getQCValues(accession, db_.processedSampleId(processed_sample_name_));
		std::sort(values.begin(), values.end());
		double median = BasicStatistics::median(values, false);
		double mad = 1.428 * BasicStatistics::mad(values, median);
		label->setToolTip("mean: " + QString::number(median, 'f', decimal_places) + "<br>stdev: " + QString::number(mad, 'f', decimal_places));

		//mark QC value red if it is an outlier
		bool ok = false;
		double value = value_str.toDouble(&ok);
		if (!ok || (label_outlier_low && value < median-2*mad) || (label_outlier_high && value > median+2*mad))
		{
			label->setText("<font color='red'>"+value_str+"</font>");
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
