#include "MainWindow.h"
#include "Settings.h"
#include "GffData.h"
#include "SharedData.h"
#include <QStyleFactory>
#include "AboutDialog.h"
#include "htslib/hts.h"
#include <QMessageBox>

MainWindow::MainWindow(QWidget *parent)
	: QMainWindow(parent)
	, ui_()
{
	ui_.setupUi(this);

	//sginals and slots
	connect(ui_.actionLoadFile, SIGNAL(triggered()), ui_.gvw, SLOT(openFileDialog()));
	connect(ui_.actionReloadTracks, SIGNAL(triggered()), ui_.gvw, SLOT(reloadTracks()));
	connect(ui_.actionNewSession, SIGNAL(triggered()), ui_.gvw, SLOT(newSession()));
	connect(ui_.actionStore_session, SIGNAL(triggered()), ui_.gvw, SLOT(saveSession()));
	connect(ui_.actionLoadSession, SIGNAL(triggered()), ui_.gvw, SLOT(loadSession()));
	connect(ui_.actionAbout, SIGNAL(triggered()), this, SLOT(showAboutDialog()));
	connect(ui_.actionExit, SIGNAL(triggered()), this, SLOT(close()));

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
	dlg.addLibVersionLine("Genome build: GRCh38");
	dlg.exec();
}
