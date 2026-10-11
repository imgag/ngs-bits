#include "MainWindow.h"
#include "Settings.h"
#include "GffData.h"
#include "SharedData.h"
#include "AboutDialog.h"
#include "htslib/hts.h"
#include "GenomeVisualizationWidget.h"

MainWindow::MainWindow(QWidget *parent)
	: QMainWindow(parent)
	, ui_()
	, init_timer_(this, true)
{
	ui_.setupUi(this);

	//sginals and slots
	connect(ui_.actionLoadFile, &QAction::triggered, ui_.gvw, &GenomeVisualizationWidget::openFileDialog);
	connect(ui_.actionLoadURL, &QAction::triggered, ui_.gvw, &GenomeVisualizationWidget::openUrlDialog);
	connect(ui_.actionReloadTracks, &QAction::triggered, ui_.gvw, &GenomeVisualizationWidget::reloadTracks);
	connect(ui_.actionClearSession, &QAction::triggered, ui_.gvw, &GenomeVisualizationWidget::clearSession);
	connect(ui_.actionStore_session, &QAction::triggered, ui_.gvw, &GenomeVisualizationWidget::saveSession);
	connect(ui_.actionLoadSession, &QAction::triggered, ui_.gvw, &GenomeVisualizationWidget::loadSession);
	connect(ui_.actionAbout, &QAction::triggered, this, &MainWindow::showAboutDialog);
	connect(ui_.actionExit, &QAction::triggered, this, &MainWindow::close);

	//Set environment variable containing SSL certificates - needed for HTTPS to work for BamReader/htslib
	QString curl_ca_bundle = Settings::string("curl_ca_bundle", true);
	if (!curl_ca_bundle.isEmpty())
	{
		if (!qputenv("CURL_CA_BUNDLE", curl_ca_bundle.toUtf8()))
		{
			qDebug() << "Could not set CURL_CA_BUNDLE variable, access to BAM/CRAM files over HTTPS may not be possible";
		}
	}
}

void MainWindow::delayedInitialization()
{
	//load transcripts from GFF
	try
	{
		QElapsedTimer timer;
		timer.start();
		{
			GffSettings settings;
			settings.print_to_stdout = false;
			GffData data = GffData::load(Settings::string("ensembl_gff"), settings);
			SharedData::setTranscripts(data.transcripts);
		}
		qDebug() << "Parsing transcripts took: " << Helper::elapsedTime(timer);
    }
	catch (Exception e)
	{
		QTextStream(stderr) << "Error loading transcripts: "+e.message();
        exit(-1);
	}
}

void MainWindow::showAboutDialog()
{
	AboutDialog dlg(this);
	dlg.setIcon(QPixmap(":/Icons/Icon.png"));
	dlg.setDescription("A free viewer for sequencing data.<br>Check the <a href='https://github.com/imgag/ngs-bits/blob/master/doc/GSviewer/index.md'>GitHub page</a> for details.");
	dlg.addLibVersionLine("htslib version: " + QString(hts_version()));
	dlg.exec();
}
