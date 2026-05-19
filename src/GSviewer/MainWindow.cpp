#include "MainWindow.h"
#include "Settings.h"
#include "GffData.h"

MainWindow::MainWindow(QWidget *parent)
	: QMainWindow(parent)
	, ui_()
    , genome_data_(new GenomeData())
{
    ui_.setupUi(this);

	try
	{
        QElapsedTimer timer;
        timer.start();

		//load transcripts from GFF
        {
            GffSettings gff_settings;
            gff_settings.source = "ensembl";
            gff_settings.include_all = false;
            gff_settings.skip_not_hgnc = false;
            gff_settings.print_to_stdout = true;
            GffData data = GffData::load(Settings::string("ensembl_gff", false), gff_settings);
            genome_data_->setTranscripts(data.transcripts);
        }
        qDebug() << "Parsing transcripts took: " << Helper::elapsedTime(timer);

        ui_.gvw->setGenomeData(genome_data_);
        ui_.gvw->setRegion("chr17", 43042292, 43172245);
    }
	catch (Exception e)
	{
		qDebug() << e.message();
        exit(-1);
	}
}
