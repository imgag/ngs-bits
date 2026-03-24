#include "MainWindow.h"
#include "Settings.h"
#include "GffData.h"
#include "SharedData.h"
#include <QFileDialog>
#include <QStyleFactory>

MainWindow::MainWindow(QWidget *parent)
	: QMainWindow(parent)
	, ui_()
{
	ui_.setupUi(this);

	//sginals and slots
	connect(ui_.actionLoadFile, SIGNAL(triggered()), ui_.gvw, SLOT(loadFile()));

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

		SharedData::setRegion("chr17", 43091889, 43093530);
    }
	catch (Exception e)
	{
		qDebug() << e.message();
        exit(-1);
	}
}
