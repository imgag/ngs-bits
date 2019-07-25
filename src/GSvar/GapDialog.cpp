#include "GapDialog.h"
#include "ui_GapDialog.h"
#include "NGSD.h"
#include "Statistics.h"
#include "ReportWorker.h"
#include "Log.h"
#include "Helper.h"
#include "GUIHelper.h"
#include <QComboBox>
#include <QFileInfo>
#include <QMessageBox>

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
	ui->percentage->setText("Percentage of target region with depth&lt;" + QString::number(cutoff) + "x: " + gap_perc + "%");

	//calculate average coverage for gaps
	Statistics::avgCoverage(low_cov, bam_file, 1, false, true);

	//update data structure
	gaps_.clear();
	GeneSet genes_noncoding;
	for(int i=0; i<low_cov.count(); ++i)
	{
		GapInfo info;
		info.line = low_cov[i];
		info.avg_depth = low_cov[i].annotations()[0].toDouble();
		info.genes = db.genesOverlappingByExon(info.line.chr(), info.line.start(), info.line.end(), 30);

		//use longest coding transcript(s) of CCDS
		BedFile ccds_overlap;
		foreach(QByteArray gene, info.genes)
		{
			int gene_id = db.geneToApprovedID(gene);
			Transcript transcript = db.longestCodingTranscript(gene_id, Transcript::ENSEMBL, true);
			if (!transcript.isValid())
			{
				genes_noncoding.insert(gene);
				transcript = db.longestCodingTranscript(gene_id, Transcript::ENSEMBL, true, true);
			}
			ccds_overlap.add(transcript.regions());
		}
		ccds_overlap.extend(5);
		BedFile current_gap;
		current_gap.append(info.line);
		ccds_overlap.intersect(current_gap);
		if (ccds_overlap.baseCount()>0)
		{
			info.ccds_overlap = ccds_overlap[0];

			if (ccds_overlap.count()>1)
			{
				Log::warn("GSvar gap dialog: one gap split into to CCDS gaps. Only using the first gap!");
			}
		}

		gaps_.append(info);
	}

	//show warning if non-coding transcripts had to be used
	if (!genes_noncoding.isEmpty())
	{
		QMessageBox::warning(this, "Non-coding transcrips were used for CCDS gaps!", "No coding transcript is defined for the following genes (for GRCh37):\n"+genes_noncoding.join(", ")+"\nFor these genes the longest *non-coding* transcript is used.\nPlease check gaps of these genes manually since they might be non-coding but shown as CCDS+-5!");
	}

	//update GUI
	ui->gaps->setRowCount(gaps_.count());
	for(int i=0; i<gaps_.count(); ++i)
	{
		//gap
		const GapInfo& gap = gaps_[i];
		ui->gaps->setItem(i, 0, createItem(gap.line.toString(true)));

		//size
		QString size = QString::number(gap.line.length());
		if (gap.isExonicSplicing()) size += " (" + QString::number(gap.ccds_overlap.length()) + ")";
		ui->gaps->setItem(i, 1, createItem(size, false, true));

		//depth
		QString depth = QString::number(gap.avg_depth, 'f', 2);
		ui->gaps->setItem(i, 2, createItem(depth, gap.avg_depth<10, true));

		//genes
		ui->gaps->setItem(i, 3, createItem(gap.genes.join(", "), gap.genes.intersectsWith(genes)));

		//type
		ui->gaps->setItem(i, 4, createItem(gap.isExonicSplicing() ? "exonic/splicing" : "intronic/intergenic" , gap.isExonicSplicing()));

		//validate
		ui->gaps->setItem(i, 5, new QTableWidgetItem());
		GapValidationLabel* label = new GapValidationLabel();
		ui->gaps->setCellWidget(i, 5, label);
		if(!gap.isExonicSplicing()) label->setState(GapValidationLabel::NO_VALIDATION);
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
	stream << "Luecken-Report\n";
	stream << "\n";
	stream << "Probe: " << sample_name_ << "\n";
	stream << "Zielregion: " << QFileInfo(roi_file_).fileName().replace(".bed", "") << "\n";
	stream << "\n";

	//exonic/splicing report
	stream << "### Regionen mit eingeschraenkter diagnostischer Beurteilbarkeit (CCDS+-5) ###\n";
	stream << "\n";
	reportSection(stream, true);

	//complete report
	stream << "\n";
	stream << "### Regionen mit eingeschraenkter diagnostischer Beurteilbarkeit (gesamte Zielregion) ###\n";
	stream << "\n";
	reportSection(stream, false);

	stream.flush();
	return  output;
}


void GapDialog::reportSection(QTextStream& stream, bool ccds_only) const
{
	stream << "Sequenzabschnitte, die mittels Sanger-Sequenzierung analysiert wurden:\n";
	int closed_sanger = 0;
	for (int row=0; row<ui->gaps->rowCount(); ++row)
	{
		const GapInfo& gap = gaps_[row];

		if (ccds_only && !gap.isExonicSplicing()) continue;

		if (state(row)==GapValidationLabel::VALIDATION)
		{
			stream << gap.asTsv(ccds_only) << "\n";
			closed_sanger += ccds_only ? gap.ccds_overlap.length() : gap.line.length();
		}
	}
	stream << "\n";

	stream << "Sequenzabschnitte, die mittels visueller Inspektion analysiert wurden";
	if (!ccds_only) stream << " (or intronic/intergenic)";
	stream << ":\n";
	int closed_manual = 0;
	for (int row=0; row<ui->gaps->rowCount(); ++row)
	{
		const GapInfo& gap = gaps_[row];

		if (ccds_only && !gap.isExonicSplicing()) continue;

		if (state(row)==GapValidationLabel::NO_VALIDATION)
		{
			stream << gap.asTsv(ccds_only) << "\n";
			closed_manual += ccds_only ? gap.ccds_overlap.length() : gap.line.length();
		}
	}
	stream << "\n";

	stream << "Sequenzabschnitte, die nicht analysiert wurden:\n";
	int closed_not = 0;
	for (int row=0; row<ui->gaps->rowCount(); ++row)
	{
		const GapInfo& gap = gaps_[row];

		if (ccds_only && !gap.isExonicSplicing()) continue;

		if (state(row)==GapValidationLabel::CHECK)
		{
			stream << gap.asTsv(ccds_only) << "\n";
			closed_not += ccds_only ? gap.ccds_overlap.length() : gap.line.length();
		}
	}
	stream << "\n";

	stream << "Zusammenfassung:\n";
	stream << "Sequenzabschnitte, die mittels Sanger-Sequenzierung analysiert wurden: " << closed_sanger << " Basen\n";
	stream << "Sequenzabschnitte, die mittels visueller Inspektion analysiert wurden: " << closed_manual << " Basen\n";
	stream << "Sequenzabschnitte, die nicht analysiert wurden: " << closed_not << " Basen\n";
	stream << "Summe: " << (closed_sanger+closed_manual+closed_not) << " Basen\n";
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

QTableWidgetItem* GapDialog::createItem(QString text, bool highlight, bool align_right)
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
