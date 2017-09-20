#include "GapDialog.h"
#include "ui_GapDialog.h"
#include "NGSD.h"
#include "Statistics.h"
#include "ReportWorker.h"
#include "Log.h"
#include "Helper.h"
#include "GUIHelper.h"
#include <QDebug>
#include <QComboBox>
#include <QFileInfo>

GapDialog::GapDialog(QWidget *parent, QString sample_name, QString roi_file)
	: QDialog(parent)
	, sample_name_(sample_name)
	, roi_file_(roi_file)
	, ui(new Ui::GapDialog)
{
	ui->setupUi(this);
	setWindowTitle("Gaps of sample " + sample_name);

	connect(ui->gaps, SIGNAL(itemDoubleClicked(QTableWidgetItem*)), this, SLOT(gapDoubleClicked(QTableWidgetItem*)));
	connect(ui->filter_gene, SIGNAL(textEdited(QString)), this, SLOT(updateGeneFilter(QString)));
}

void GapDialog::process(QString bam_file, const BedFile& roi, const GeneSet& genes)
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
	Statistics::avgCoverage(low_cov, bam_file, 1, false, true);

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
		QString depth = line.annotations()[0];
		bool highlight_depth = depth.toDouble()<10;
		ui->gaps->setItem(i, 2, createItem(depth, highlight_depth, true));

		//genes
		GeneSet genes_anno = db.genesOverlappingByExon(line.chr(), line.start(), line.end(), 30);
		bool highlight_genes = genes_anno.intersectsWith(genes);
		ui->gaps->setItem(i, 3, createItem(genes_anno.join(", "), highlight_genes));

		//type
		GeneSet genes_core = db.genesOverlappingByExon(line.chr(), line.start(), line.end(), 5);
		QString type = genes_core.count()==0 ? "intronic/intergenic" : "exonic/splicing";
		bool highlight_type = genes_core.count();
		ui->gaps->setItem(i, 4, createItem(type, highlight_type));

		//validate
		ui->gaps->setItem(i, 5, new QTableWidgetItem());
		GapValidationLabel* label = new GapValidationLabel();
		ui->gaps->setCellWidget(i, 5, label);
		if(!highlight_type) label->setState(GapValidationLabel::NO_VALIDATION);
	}

	GUIHelper::resizeTableCells(ui->gaps);
}

GapDialog::~GapDialog()
{
	delete ui;
}

QString GapDialog::report() const
{
	QString output;
	QTextStream stream(&output);

	//header
	stream << "Gap report\n";
	stream << "\n";
	stream << "Sample: " << sample_name_ << "\n";
	stream << "Target region: " << QFileInfo(roi_file_).fileName().replace(".bed", "") << "\n";
	stream << "\n";

	//gaps (sanger)
	stream << "Gaps to be closed by sanger sequencing:\n";
	int closed_sanger = 0;
	for (int row=0; row<ui->gaps->rowCount(); ++row)
	{
		if (state(row)==GapValidationLabel::VALIDATION)
		{
			stream << gapAsTsv(row) << "\n";
			closed_sanger += gapSize(row);
		}
	}
	stream << "\n";

	//gaps (IGV)
	stream << "Gaps closed by manual inspection (or intronic/intergenic):\n";
	int closed_manual = 0;
	for (int row=0; row<ui->gaps->rowCount(); ++row)
	{
		if (state(row)==GapValidationLabel::NO_VALIDATION)
		{
			stream << gapAsTsv(row) << "\n";
			closed_manual += gapSize(row);
		}
	}
	stream << "\n";

	stream << "Gaps closed by sanger sequencing: " << closed_sanger << " bases\n";
	stream << "Gaps closed by manual inspection: " << closed_manual << " bases\n";
	stream << "Sum: " << (closed_sanger+closed_manual) << " bases\n";

	stream.flush();
	return  output;
}

void GapDialog::gapDoubleClicked(QTableWidgetItem* item)
{
	if (item==nullptr) return;

	QString region = ui->gaps->item(item->row(), 0)->text();
	emit openRegionInIGV(region);
}

void GapDialog::updateGeneFilter(QString text)
{
	for (int i=0; i<ui->gaps->rowCount(); ++i)
	{
		bool hide = !ui->gaps->item(i, 3)->text().contains(text, Qt::CaseInsensitive);
		ui->gaps->setRowHidden(i, hide);
	}
}

QTableWidgetItem*GapDialog::createItem(QString text, bool highlight, bool align_right)
{
	QTableWidgetItem* item = new QTableWidgetItem(text);
	if (highlight)
	{
		QFont font;
		font.setBold(true);
		item->setFont(font);
	}
	item->setFlags(Qt::ItemIsSelectable|Qt::ItemIsEnabled);
	if (align_right) item->setTextAlignment(Qt::AlignRight|Qt::AlignCenter);
	return item;
}

GapValidationLabel::State GapDialog::state(int row) const
{
	return qobject_cast<GapValidationLabel*>(ui->gaps->cellWidget(row, 5))->state();
}

QString GapDialog::gapAsTsv(int row) const
{
	return ui->gaps->item(row, 0)->text().replace("-", "\t").replace(":", "\t") + "\t" + ui->gaps->item(row, 3)->text();
}

int GapDialog::gapSize(int row) const
{
	return Helper::toInt(ui->gaps->item(row, 1)->text(), "gap size");
}

