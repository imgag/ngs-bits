#include "MainWindow.h"
#include "Settings.h"
#include "GffData.h"
#include "SharedData.h"
#include <QFileDialog>
#include <QMessageBox>
#include <QRegularExpression>
#include <QStyleFactory>
#include <RefGenomeService.h>
#include "AboutDialog.h"
#include "htslib/hts.h"
#include "GenomeVisualizationWidget.h"
#include <QApplication>
#include <QProgressDialog>

MainWindow::MainWindow(QWidget *parent)
	: QMainWindow(parent)
	, ui_()
	, init_timer_(this, true)
{
	ui_.setupUi(this);

    // signals and slots
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

    // API
    connect(&api, &CommandServer::commandReceived, this, &MainWindow::executeApiCommand);
}

void MainWindow::delayedInitialization()
{
	//load transcripts from GFF
	try
	{
		QElapsedTimer timer;
		timer.start();
		{
			QProgressDialog loading_dialog("loading gene/transcript Gff file", QString(), 0, 0, this);
			loading_dialog.setWindowModality(Qt::ApplicationModal);
			loading_dialog.setCancelButton(nullptr);
			loading_dialog.setMinimumDuration(0);
			loading_dialog.show();
			QApplication::processEvents(QEventLoop::ExcludeUserInputEvents);

			GffSettings settings;
			settings.print_to_stdout = false;
			GffData data = GffData::load(Settings::string("ensembl_gff"), settings);
			SharedData::setTranscripts(data.transcripts);
		}
        qDebug() << "Parsing transcripts took: " << Helper::elapsedTime(timer);

        bool enableApi = Settings::boolean("enable_remote_application_control", false);

        if (enableApi)
            ui_.actionEnable_Remote_Application_Control->setChecked(true);

        SharedData::setRegion("chr17", 43091889, 43093530);
    }
	catch (Exception e)
	{
		QTextStream(stderr) << "Error loading transcripts: "+e.message();
        exit(-1);
	}
}

void MainWindow::on_actionEnable_Remote_Application_Control_changed()
{
    bool shouldEnable = ui_.actionEnable_Remote_Application_Control->isChecked();

    if (shouldEnable)
        api.bind();

    else
        api.terminate();
}

void MainWindow::executeApiCommand(GSVCommand cmd) {
    qDebug() << "Received command:" << cmd.text.trimmed().toStdString();

    try {
        switch (cmd.verb) {
        case CmdVerb::New:
            ui_.gvw->newSession();
            break;

        case CmdVerb::Goto:
            handleGoto(cmd.arguments);
            break;

        case CmdVerb::Load:
            handleLoad(cmd.arguments);
            break;

        case CmdVerb::Genome:
            handleGenome(cmd.arguments);
            break;

        default:
            break;
        }
    } catch (Exception e) {
        err("An invalid command was received", e.message());
    }
}

void MainWindow::err(QString header, QString text) {
    auto *box = new QMessageBox(QMessageBox::Warning, header, text, QMessageBox::Ok, this);
    box->setAttribute(Qt::WA_DeleteOnClose);
    box->open();   // window-modal, returns immediately
}

void MainWindow::handleGoto(QString args)
{
    const QRegularExpression re("[,;]");

    auto tracks = args.split(re);

    for (QString locus : tracks)
    {
        BedLine region = BedLine::fromString(locus);
        if (region.isValid())
        {
            SharedData::setRegion(region.chr(), region.start(), region.end());
            return;
        }
        else
        {
            err("Invalid region", "The region specified is not valid and can therefore not be used as a destination. If you meant to jump to a particular coordinate, be sure to include the chromosome and start/end positions in the format (chrN:start-end), or just the chromonose name: (chrN).");
        }
    }
}

void MainWindow::handleLoad(QString args)
{
    if (args.trimmed().isEmpty())
        ui_.gvw->openFileDialog();

    else
    {
        const QRegularExpression re("[,;]");

        for (QString file : args.split(re))
            ui_.gvw->loadFile(file);
    }
}

void MainWindow::handleGenome(QString args) {
    QString file = args;

    if (args.trimmed().isEmpty())
    {
        QString open_folder = Settings::path("load_store_file_folder", true);
        file = QFileDialog::getOpenFileName(this, "Open file(s)", open_folder, "Genome files(*.fa, *.fa.gz);;All files(*.*)");
    }

    ui_.gvw->newSession();
    RefGenomeService::instance().setReferenceGenome(file);
}
void MainWindow::showAboutDialog()
{
	AboutDialog dlg(this);
	dlg.setIcon(QPixmap(":/Icons/Icon.png"));
	dlg.setDescription("A free viewer for sequencing data.<br>Check the <a href='https://github.com/imgag/ngs-bits/blob/master/doc/GSviewer/index.md'>GitHub page</a> for details.");
	dlg.addLibVersionLine("htslib version: " + QString(hts_version()));
    dlg.exec();
}
