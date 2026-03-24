#include "MainWindow.h"
#include "Settings.h"
#include "GffData.h"
#include "SharedData.h"
#include <QFileDialog>

MainWindow::MainWindow(QWidget *parent)
	: QMainWindow(parent)
	, ui_()
{
	ui_.setupUi(this);
	connect(this, SIGNAL(loadFile()), ui_.gvw, SIGNAL(loadFile()));

	try
	{
		//load transcripts from GFF
		createMenus();
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

		SharedData::setRegion("chr17", 43091889, 43093530);
    }
	catch (Exception e)
	{
		qDebug() << e.message();
        exit(-1);
	}
}


void MainWindow::createMenus()
{
	file_menu_ = menuBar()->addMenu(tr("&File"));
	QAction* action = file_menu_->addAction("Load File");
	connect(action, SIGNAL(triggered()), this, SIGNAL(loadFile()));
}
