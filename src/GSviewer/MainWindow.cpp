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
#include <QMessageBox>

MainWindow::MainWindow(QWidget *parent)
	: QMainWindow(parent)
	, ui_()
	, init_timer_(this, true)
{
	ui_.setupUi(this);

    // signals and slots
	connect(ui_.actionLoadFile, SIGNAL(triggered()), ui_.gvw, SLOT(openFileDialog()));
	connect(ui_.actionReloadTracks, SIGNAL(triggered()), ui_.gvw, SLOT(reloadTracks()));
	connect(ui_.actionNewSession, SIGNAL(triggered()), ui_.gvw, SLOT(newSession()));
	connect(ui_.actionStore_session, SIGNAL(triggered()), ui_.gvw, SLOT(saveSession()));
    connect(ui_.actionLoadSession, SIGNAL(triggered()), ui_.gvw, SLOT(loadSession()));
	connect(ui_.actionAbout, SIGNAL(triggered()), this, SLOT(showAboutDialog()));
	connect(ui_.actionExit, SIGNAL(triggered()), this, SLOT(close()));

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
	dlg.addLibVersionLine("Genome build: GRCh38");
    dlg.exec();
}
