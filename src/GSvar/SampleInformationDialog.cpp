#include "SampleInformationDialog.h"
#include "GUIHelper.h"
#include "Exceptions.h"
#include "Log.h"
#include "BasicStatistics.h"
#include <QFileInfo>
#include <QUrl>
#include <QMenu>
#include <QMessageBox>
#include <QDateTime>
#include "HttpHandler.h"
#include "Settings.h"
#include "Helper.h"
#include "DiagnosticStatusWidget.h"

SampleInformationDialog::SampleInformationDialog(QWidget* parent, QString processed_sample_name)
	: QDialog(parent)
	, ui_()
	, processed_sample_name_(processed_sample_name)
	, reanalyze_status_()
{
	//set busy cursor
	QApplication::setOverrideCursor(QCursor(Qt::BusyCursor));

	//setup of UI
	ui_.setupUi(this);

	//setup reanalyze button
	QMenu* menu = new QMenu();
	menu->addAction("start at mapping (recommended)", this, SLOT(reanalyze()));
	menu->addAction("start at variant calling", this, SLOT(reanalyze()));
	menu->addAction("start at annotation", this, SLOT(reanalyze()));
	menu->addAction("start at copy-number analysis", this, SLOT(reanalyze()));
	ui_.reanalyze_button->setMenu(menu);

	//diagnostic status button
	connect(ui_.diag_status_button, SIGNAL(clicked(bool)), this, SLOT(editDiagnosticStatus()));

	//setup quality button
	NGSD db;
	if (db.isOpen())
	{
		menu = new QMenu();
		QStringList quality = db.getEnum("processed_sample", "quality");
		foreach(QString q, quality)
		{
			menu->addAction(q, this, SLOT(setQuality()));
		}
		ui_.quality_button->setMenu(menu);
	}
	else
	{
		ui_.quality_button->setEnabled(false);
	}

	//refresh
	refresh();

	//init timer for refresh of reanalysis button
	timer_.setSingleShot(false);
	connect(&timer_, SIGNAL(timeout()), this, SLOT(refreshReanalysisStatus()));
	timer_.start(5000);

	//reset busy cursor
	QApplication::restoreOverrideCursor();
}

void SampleInformationDialog::reanalyze()
{
	//determine start step from action
	QString start_step = "";
	QAction* action = qobject_cast<QAction*>(sender());
	if (action->text().contains("mapping"))
	{
		start_step = "&start_step=ma";
	}
	else if (action->text().contains("variant calling"))
	{
		start_step = "&start_step=vc";
	}
	else if (action->text().contains("annotation"))
	{
		start_step = "&start_step=an";
	}
	else if (action->text().contains("copy-number"))
	{
		start_step = "&start_step=cn";
	}
	else
	{
		THROW(ProgrammingException, "Unknown reanalysis action: " + action->text());
	}

	//call web service
	QString reply = HttpHandler(HttpHandler::NONE).getHttpReply(Settings::string("SampleStatus")+"/restart.php?type=sample&high_priority&user=" + Helper::userName() + "&ps_ID=" + processed_sample_name_ + start_step);
	reanalyze_status_ = "";
	if (!reply.startsWith("Restart successful"))
	{
		reanalyze_status_ = "restart of analysis failed: " + reply;
	}

	//refresh
	refresh();
}

void SampleInformationDialog::editDiagnosticStatus()
{
	NGSD db;
	QString processed_sample_id = db.processedSampleId(processed_sample_name_);

	DiagnosticStatusWidget* widget = new DiagnosticStatusWidget(this);
	widget->setStatus(db.getDiagnosticStatus(processed_sample_id));
	auto dlg = GUIHelper::showWidgetAsDialog(widget, "Diagnostic status of " + processed_sample_name_, true);
	if (dlg->result()!=QDialog::Accepted) return;

	db.setDiagnosticStatus(processed_sample_id, widget->status());
	refresh();
}

void SampleInformationDialog::setQuality()
{
	QString quality = qobject_cast<QAction*>(sender())->text();
	NGSD db;
	QString processed_sample_id = db.processedSampleId(processed_sample_name_);
	db.setProcessedSampleQuality(processed_sample_id, quality);
	refresh();
}

void SampleInformationDialog::refresh()
{
	NGSD db;

	//sample data
	ui_.name->setText(processed_sample_name_);
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

	//processed sample data
	try
	{
		//QC data
		QString processed_sample_id = db.processedSampleId(processed_sample_name_);
		ProcessedSampleData processed_sample_data = db.getProcessedSampleData(processed_sample_id);
		QCCollection qc = db.getQCData(processed_sample_id);
		ui_.qc_reads->setText(qc.value("QC:2000005", true).toString(0) + " (length: " + qc.value("QC:2000006", true).toString(0) + ")");

		statisticsLabel(db, ui_.qc_avg_depth, "QC:2000025", qc, true, false);
		statisticsLabel(db, ui_.qc_perc_20x, "QC:2000027", qc, true, false);
		statisticsLabel(db, ui_.qc_insert_size,"QC:2000023", qc, true, true);
		statisticsLabel(db, ui_.qc_af_dev,"QC:2000051", qc, false, true, 4);

		ui_.qc_kasp->setText(qc.value("kasp").asString());
		QString quality = processed_sample_data.quality;
		if (quality=="bad") quality = "<font color='red'>"+quality+"</font>";
		if (quality=="medium") quality = "<font color='orange'>"+quality+"</font>";
		ui_.qc_quality->setText(quality);
		ui_.comment->setText(processed_sample_data.comments);
		ui_.system->setText(processed_sample_data.processing_system);

		//diagnostic status
		DiagnosticStatusData diag_status = db.getDiagnosticStatus(processed_sample_id);
		ui_.diag_status->setText(diag_status.dagnostic_status);
		ui_.diag_outcome->setText(diag_status.outcome);

		//last analysis
		QDateTime last_modified = QFileInfo(db.processedSamplePath(processed_sample_id, NGSD::BAM)).lastModified();
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

	//reanalysis status
	refreshReanalysisStatus();
}


void SampleInformationDialog::statisticsLabel(NGSD& db, QLabel* label, QString accession, const QCCollection& qc, bool label_outlier_low, bool label_outlier_high, int decimal_places)
{
	try
	{
		//get QC value
		QString value_str = qc.value(accession, true).toString();
		label->setText(value_str);

		//get QC value statistics
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

void SampleInformationDialog::refreshReanalysisStatus()
{
	QString ps_name = NGSD::processedSampleName(processed_sample_name_, false);
	if (ps_name!="")
	{
		QString status = reanalyze_status_;
		if (status=="")
		{
			try
			{
				status = HttpHandler(HttpHandler::NONE).getHttpReply(Settings::string("SampleStatus")+"/status.php?ps_ID=" + ps_name);
			}
			catch (Exception& e)
			{
				Log::warn("Could not connect to SampleStatus web service!");
			}
		}
		if (status=="unknown")
		{
			ui_.status->setText("n/a (no status available)");
		}
		else if (status=="running" || status=="queued" || status=="finished")
		{
			ui_.status->setText("<font color='green'>" + status + "</font>");
		}
		else if (status=="low coverage")
		{
			ui_.status->setText("<font color='orange'>" + status + "</font>");
		}
		else if (status=="warnings")
		{
			ui_.status->setText("<a href=\""+Settings::string("SampleStatus")+"/index.php\"><font color='orange'>" + status + "</font></a>");
		}
		else
		{
			ui_.status->setText("<a href=\""+Settings::string("SampleStatus")+"/index.php\"><font color='red'>" + status + "</font></a>");
		}

		//reanalysis button
		ui_.reanalyze_button->setEnabled(status!="running" && status!="queued");
	}
	else
	{
		ui_.status->setText("<font color='red'>n/a (invalid sample file name)</font>");
		ui_.reanalyze_button->setEnabled(false);
	}
}
