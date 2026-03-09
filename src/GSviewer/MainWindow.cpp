#include "MainWindow.h"
#include "Settings.h"
#include "NGSHelper.h"

MainWindow::MainWindow(QWidget *parent)
	: QMainWindow(parent)
	, ui_()
{
	QElapsedTimer timer;
	timer.start();
	try
	{
		//load transcripts from GFF
		GffSettings gff_settings;
		gff_settings.source = "ensembl";
		gff_settings.include_all = false;
		gff_settings.skip_not_hgnc = false;
		gff_settings.print_to_stdout = true;
		GffData data = NGSHelper::loadGffFile(Settings::string("ensembl_gff", false), gff_settings);
		qDebug() << "Parsing transcripts took: " << Helper::elapsedTime(timer) << Qt::endl;
		ui_.gvw->setTranscripts(data.transcripts);
		timer.restart();
		qDebug() << "initializing transcripts took: " << Helper::elapsedTime(timer) << Qt::endl;
	}
	catch (Exception e)
	{
		qDebug() << e.message();
		exit(-1);
	}
}



