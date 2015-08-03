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
	, db_()
	, filename_(filename)
	, reanalyze_status_()
{
	//set busy cursor
	QApplication::setOverrideCursor(QCursor(Qt::BusyCursor));

	//setup of UI
	ui_.setupUi(this);
	ui_.status->setOpenExternalLinks(true);

	//set up reanalyze button
	QMenu* menu = new QMenu();
	menu->addAction("start at mapping (recommended)", this, SLOT(reanalyze()));
	menu->addAction("start at variant calling", this, SLOT(reanalyze()));
	menu->addAction("start at annotation", this, SLOT(reanalyze()));
	ui_.reanalyze_button->setMenu(menu);

	menu = new QMenu();
	QStringList status = db_.getEnum("diag_status", "status");
	foreach(QString s, status)
	{
		menu->addAction(s, this, SLOT(setReportStatus()));
	}
	ui_.report_button->setMenu(menu);

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
	else
	{
		THROW(ProgrammingException, "Unknown reanalysis action: " + action->text());
	}

	//call web service
	HttpHandler handler;
	QString reply = handler.getHttpReply(Settings::string("SampleStatus")+"/restart.php?ps_ID=" + psName(filename_) + start_step + "&high_priority&user=" + Helper::userName());
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

	db_.setDiagnosticStatus(filename_, status);
	refresh();
}

void SampleInformationDialog::refresh()
{
	//name
	ui_.name->setText(QFileInfo(filename_).fileName());

	//date - BAM
	QDateTime last_modified = QFileInfo(QString(filename_).replace(".GSvar", ".bam")).lastModified();
	QString date_text = last_modified.toString("yyyy-MM-dd");
	if (last_modified < QDateTime::currentDateTime().addMonths(-6))
	{
		date_text = " <font color='red'>" + date_text + "</font>";
	}
	ui_.date_bam->setText(date_text);

	//date - VCF
	last_modified = QFileInfo(QString(filename_).replace(".GSvar", "_var.vcf")).lastModified();
	date_text = last_modified.toString("yyyy-MM-dd");
	if (last_modified < QDateTime::currentDateTime().addMonths(-6))
	{
		date_text = " <font color='red'>" + date_text + "</font>";
	}
	ui_.date_vcf->setText(date_text);

	//date - GSvar
	last_modified = QFileInfo(filename_).lastModified();
	date_text = last_modified.toString("yyyy-MM-dd");
	if (last_modified < QDateTime::currentDateTime().addMonths(-2))
	{
		date_text = " <font color='red'>" + date_text + "</font>";
	}
	ui_.date_gsvar->setText(date_text);

	//QC data
	QCCollection qc = db_.getQCData(filename_);
	try
	{
		ui_.qc_reads->setText(qc.value("QC:2000005", true).toString(0) + " (length: " + qc.value("QC:2000006", true).toString(0) + ")");
	}
	catch (Exception& e)
	{
		ui_.qc_reads->setText("<font color=\"red\">n/a</font>");
		ui_.qc_reads->setToolTip("NGSD error: " + e.message());
	}
	statisticsLabel(ui_.qc_avg_depth, "QC:2000025", qc);
	statisticsLabel(ui_.qc_perc_20x, "QC:2000027", qc);
	statisticsLabel(ui_.qc_insert_size,"QC:2000023", qc );
	ui_.qc_kasp->setText(qc.value("kasp").asString());
	//processing system
	try
	{
		ui_.system->setText(db_.getProcessingSystem(filename_, NGSD::LONG));
	}
	catch (DatabaseException&)
	{
		ui_.system->setText("DB error");
	}

	//reanalysis status
	refreshReanalysisStatus();

	//report status
	try
	{
		QStringList diag_status = db_.getDiagnosticStatus(filename_);
		if (diag_status.count()!=4)
		{
			qDebug() << "Attention, getDiagnosticStatus element count != 4!";
			throw DatabaseException("", "", -1);
		}
		ui_.report_status->setText(diag_status[0]);
		ui_.report_user->setText(diag_status[1]);
		ui_.report_date->setText(diag_status[2]);
		ui_.report_outcome->setText(diag_status[3]);
	}
	catch(DatabaseException&)
	{
		ui_.report_status->setText("DB error");
		ui_.report_user->setText("DB error");
		ui_.report_date->setText("DB error");
		ui_.report_outcome->setText("DB error");
		ui_.report_button->setEnabled(false);
	}
}

void SampleInformationDialog::statisticsLabel(QLabel* label, QString accession, const QCCollection& qc)
{
	try
	{
		//get QC value
		QString value_str = qc.value(accession, true).toString();
		label->setText(value_str);

		//get QC value statistics
		QVector<double> values = db_.getQCValues(accession, filename_);
		std::sort(values.begin(), values.end());
		double median = BasicStatistics::median(values);
		double mad = 1.428 * BasicStatistics::mad(values, median);
		label->setToolTip("mean: " + QString::number(median, 'f', 2) + "<br>stdev: " + QString::number(mad, 'f', 2));

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
	QString ps_name = psName(filename_);
	if (ps_name!="")
	{
		QString status = reanalyze_status_;
		if (status=="")
		{
			HttpHandler handler;
			status = handler.getHttpReply(Settings::string("SampleStatus")+"/status.php?ps_ID=" + ps_name);
		}

		if (status=="unknown")
		{
			ui_.status->setText("not yet reanalyzed");
		}
		else if (status=="running" || status=="queued" || status=="finished")
		{
			ui_.status->setText("<font color='green'>" + status + "</font>");
		}
		else if (status=="low coverage")
		{
			ui_.status->setText("<font color='orange'>" + status + "</font>");
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

QString SampleInformationDialog::psName(QString filename)
{
	try
	{
		QString sample_name = db_.sampleName(filename);
		QString ps_id = db_.processedSampleNumber(filename);
		if (sample_name=="" || ps_id=="") return "";

		return sample_name + "_" + ps_id.rightJustified(2, '0');
	}
	catch (DatabaseException&)
	{
		return "";
	}
}
