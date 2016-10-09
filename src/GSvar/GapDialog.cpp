#include "GapDialog.h"
#include "ui_GapDialog.h"
#include "NGSD.h"
#include "Statistics.h"
#include "ReportWorker.h"
#include "Log.h"
#include <QDebug>

GapDialog::GapDialog(QWidget *parent) :
	QDialog(parent),
	ui(new Ui::GapDialog)
{
	ui->setupUi(this);
	connect(ui->gaps, SIGNAL(itemDoubleClicked(QTableWidgetItem*)), this, SLOT(gapDoubleClicked(QTableWidgetItem*)));
}

void GapDialog::setSampleName(QString sample_name)
{
	sample_name_ = sample_name;
	setWindowTitle("Gaps of sample " + sample_name);
}

void GapDialog::process(QString bam_file, const BedFile& roi, QSet<QString> genes)
{
	//init
	NGSD db;
	int cutoff = 20;

	//calculate low-coverage regions
	QString message;
	BedFile low_cov = ReportWorker::precalculatedGaps(bam_file, roi, cutoff, db, message);
	if (!message.isEmpty())
	{
		Log::warn("Low-coverage statistics needs to be calculated. Pre-calulated gap file cannot be used because: " + message);
		low_cov = Statistics::lowCoverage(roi, bam_file, cutoff);
	}

	//show percentage of gaps
	QString gap_perc = QString::number(100.0 * low_cov.baseCount() / roi.baseCount(), 'f', 2);
	ui->percentage->setText("Percentage of target region with depth&lt;" + QString::number(cutoff) + ": " + gap_perc + "%");

	//calculate average coverage for gaps
	Statistics::avgCoverage(low_cov, bam_file);

	//show gaps
	ui->gaps->setRowCount(low_cov.count());
	for(int i=0; i<low_cov.count(); ++i)
	{
		//gap
		const BedLine& line = low_cov[i];
		ui->gaps->setItem(i, 0, createItem(line.toString(true)));

		//size
		ui->gaps->setItem(i, 1, createItem(QString::number(line.length()), false, true));

		//depth
		ui->gaps->setItem(i, 2, createItem(line.annotations()[0], false, true));

		//genes
		QStringList genes_anno = db.genesOverlappingByExon(line.chr(), line.start(), line.end(), 20);
		bool highlight_genes = genes_anno.toSet().intersect(genes).count();
		ui->gaps->setItem(i, 3, createItem(genes_anno.join(", "), highlight_genes));

		//type
		QStringList genes_core = db.genesOverlappingByExon(line.chr(), line.start(), line.end(), 5);
		QString type = genes_core.count()==0 ? "intronic/intergenic" : "exonic/splicing";
		bool highlight_type = genes_core.count();
		ui->gaps->setItem(i, 4, createItem(type, highlight_type));
	}

	ui->gaps->resizeColumnsToContents();
}

GapDialog::~GapDialog()
{
	delete ui;
}

void GapDialog::gapDoubleClicked(QTableWidgetItem* item)
{
	if (item==nullptr) return;

	QString region = ui->gaps->item(item->row(), 0)->text();
	emit openRegionInIgv(region);
}

