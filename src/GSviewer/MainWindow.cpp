#include "MainWindow.h"
#include "Settings.h"
#include "GffData.h"
#include "SharedData.h"
#include <QFileDialog>
#include <QMessageBox>
#include <QRegularExpression>
#include <QStyleFactory>

MainWindow::MainWindow(QWidget *parent)
	: QMainWindow(parent)
	, ui_()
{
	ui_.setupUi(this);

	//sginals and slots
	connect(ui_.actionLoadFile, SIGNAL(triggered()), ui_.gvw, SLOT(loadFile()));
	connect(ui_.actionReloadTracks, SIGNAL(triggered()), ui_.gvw, SLOT(reloadTracks()));
	connect(ui_.actionNewSession, SIGNAL(triggered()), ui_.gvw, SLOT(newSession()));
	connect(ui_.actionStore_session, SIGNAL(triggered()), ui_.gvw, SLOT(saveSession()));
	connect(ui_.actionLoadSession, SIGNAL(triggered()), ui_.gvw, SLOT(loadSession()));
    connect(&api, &CommandServer::commandReceived, this, &MainWindow::executeApiCommand);

	//set windows 10 style
	QStyle* style = QStyleFactory::create("windowsvista");
	QApplication::setStyle(style);

	try
	{
		//load transcripts from GFF
		QElapsedTimer timer;
		timer.start();
        {
			GffSettings gff_settings;
            gff_settings.source = "ensembl";
            gff_settings.include_all = false;
            gff_settings.skip_not_hgnc = false;
            gff_settings.print_to_stdout = true;
			GffData data = GffData::load(Settings::string("ensembl_gff", false), gff_settings);
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
		qDebug() << e.message();
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

        case CmdVerb::Load:
            // for (QString arg : cmd.arguments)
            //     loadFile();

            break;

        default:
            break;
        }
    } catch (Exception err) {
        QMessageBox::critical(this, "An invalid command was received", err.message());
    }
}

void MainWindow::handleGoto(QString args) {
    QRegularExpression re("[,;]");

    auto tracks = args.split(re);

    for (QString locus : tracks)
        ui_.gvw.
}
