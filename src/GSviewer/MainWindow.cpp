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
            genome_data_->transcripts = data.transcripts;
        }
        qDebug() << "Parsing transcripts took: " << Helper::elapsedTime(timer);

        //sort and index transcripts
        timer.restart();
        genome_data_->transcripts.sortByPosition();
        genome_data_->transcript_index.createIndex();
        qDebug() << "Sorting and indexing transcripts took: " << Helper::elapsedTime(timer);

        ui_.gvw->setGenomeData(genome_data_);
        ui_.gvw->setRegion("chr17", 43044295, 43125364);
    }
	catch (Exception e)
	{
		qDebug() << e.message();
        exit(-1);
	}
}

//TODO:
//- no init region
//- icon

