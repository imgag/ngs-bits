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

SampleInformationDialog::SampleInformationDialog(QWidget* parent, QString filename)
	: QDialog(parent)
	, ui_()
	, filename_(filename)
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

	//setup report button
	NGSD db;
	if (db.isOpen())
	{
		menu = new QMenu();
		QStringList status = db.getEnum("diag_status", "status");
		foreach(QString s, status)
		{
			menu->addAction(s, this, SLOT(setReportStatus()));
		}
		ui_.report_button->setMenu(menu);
	}
	else
	{
		ui_.report_button->setEnabled(false);
	}

	//setup quality button
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
	HttpHandler handler;
	QString reply = handler.getHttpReply(Settings::string("SampleStatus")+"/restart.php?type=sample&high_priority&user=" + Helper::userName() + "&ps_ID=" + NGSD::processedSampleName(filename_) + start_step);
	reanalyze_status_ = "";
	if (!reply.startsWith("Restart successful"))
	{
		reanalyze_status_ = "restart of analysis failed: " + reply;
	}

	//refresh
	refresh();
}

void SampleInformationDialog::setReportStatus()
{
	QString status = qobject_cast<QAction*>(sender())->text();

	if (status.startsWith("repeat"))
	{
		int result = QMessageBox::question(this, "Re-sequencing of sample?", "The sample will be scheduled for re-processing in the lab.\nAre you sure?");
		if (result!=QMessageBox::Yes) return;
	}

	NGSD db;
	db.setDiagnosticStatus(filename_, status);
	refresh();
}

void SampleInformationDialog::setQuality()
{
	QString quality = qobject_cast<QAction*>(sender())->text();
	NGSD db;
	db.setProcessedSampleQuality(filename_, quality);
	refresh();
}

void SampleInformationDialog::refresh()
{
	NGSD db;

	//sample details
	ui_.name->setText(QFileInfo(filename_).fileName());
	try
	{
		ui_.name_ext->setText(db.getExternalSampleName(filename_));
		ui_.tumor_ffpe->setText(db.sampleIsTumor(filename_) + " / " + db.sampleIsFFPE(filename_));
		ui_.disease_group->setText(db.sampleDiseaseGroup(filename_));
		ui_.system->setText(db.getProcessingSystem(filename_, NGSD::LONG));
	}
	catch(...)
	{
	}

	//QC data
	try
	{
		QCCollection qc = db.getQCData(filename_);
		ui_.qc_reads->setText(qc.value("QC:2000005", true).toString(0) + " (length: " + qc.value("QC:2000006", true).toString(0) + ")");

		statisticsLabel(db, ui_.qc_avg_depth, "QC:2000025", qc);
		statisticsLabel(db, ui_.qc_perc_20x, "QC:2000027", qc);
		statisticsLabel(db, ui_.qc_insert_size,"QC:2000023", qc);
		statisticsLabel(db, ui_.qc_af_dev,"QC:2000051", qc, 4);

		ui_.qc_kasp->setText(qc.value("kasp").asString());
		ui_.qc_quality->setText(db.getProcessedSampleQuality(filename_, true));
	}
	catch(...)
	{
		ui_.quality_button->setEnabled(false);
	}

	//report generation
	try
	{
		QStringList diag_status = db.getDiagnosticStatus(filename_);
		if (diag_status.count()==4)
		{
			ui_.report_status->setText(diag_status[0]);
			ui_.report_user->setText(diag_status[1]);
			ui_.report_date->setText(diag_status[2]);
			ui_.report_outcome->setText(diag_status[3]);
		}
	}
	catch(...)
	{
		ui_.report_button->setEnabled(false);
	}

	//last analysis
	QDateTime last_modified = QFileInfo(QString(filename_).replace(".GSvar", ".bam")).lastModified();
	QString date_text = last_modified.toString("yyyy-MM-dd");
	if (last_modified < QDateTime::currentDateTime().addMonths(-6))
	{
		date_text = " <font color='red'>" + date_text + "</font>";
	}
	ui_.date_bam->setText(date_text);

	//reanalysis status
	refreshReanalysisStatus();
}


void SampleInformationDialog::statisticsLabel(NGSD& db, QLabel* label, QString accession, const QCCollection& qc, int decimal_places)
{
	try
	{
		//get QC value
		QString value_str = qc.value(accession, true).toString();
		label->setText(value_str);

		//get QC value statistics
		QVector<double> values = db.getQCValues(accession, filename_);
		std::sort(values.begin(), values.end());
		double median = BasicStatistics::median(values, false);
		double mad = 1.428 * BasicStatistics::mad(values, median);
		label->setToolTip("mean: " + QString::number(median, 'f', decimal_places) + "<br>stdev: " + QString::number(mad, 'f', decimal_places));

		//mark QC value red if it is an outlier
		bool ok = false;
		double value = value_str.toDouble(&ok);
		if (!ok || value < median-2*mad || value > median+2*mad)
		{
			label->setText("<font color=\"red\">"+value_str+"</font>");
		}
	}
	catch (ArgumentException e) //in case the QC value does not exist in the DB
	{
		label->setText("<font color=\"red\">n/a</font>");
		label->setToolTip("NGSD error: " + e.message());
	}
	catch (StatisticsException e) //in case the QC values statistics cannot be calculated
	{
		label->setToolTip("Statistics error: " + e.message());
	}
}

void SampleInformationDialog::refreshReanalysisStatus()
{
	QString ps_name = NGSD::processedSampleName(filename_, false);
	if (ps_name!="")
	{
		QString status = reanalyze_status_;
		if (status=="")
		{
			try
			{
				HttpHandler handler;
				status = handler.getHttpReply(Settings::string("SampleStatus")+"/status.php?ps_ID=" + ps_name);
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
