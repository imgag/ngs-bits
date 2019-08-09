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

GapDialog::GapDialog(QWidget *parent, QString sample_name, QString roi_file, QMap<QString, QStringList> preferred_transcripts)
	: QDialog(parent)
	, sample_name_(sample_name)
	, roi_file_(roi_file)
	, preferred_transcripts_(preferred_transcripts)
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

		//use longest coding transcript(s) of Ensembl
		BedFile coding_overlap;
		foreach(QByteArray gene, info.genes)
		{
			int gene_id = db.geneToApprovedID(gene);
			Transcript transcript = db.longestCodingTranscript(gene_id, Transcript::ENSEMBL, true);
			if (!transcript.isValid())
			{
				genes_noncoding.insert(gene);
				transcript = db.longestCodingTranscript(gene_id, Transcript::ENSEMBL, true, true);
			}
			coding_overlap.add(transcript.regions());
		}
		coding_overlap.extend(5);
		BedFile current_gap;
		current_gap.append(info.line);
		coding_overlap.intersect(current_gap);
		if (coding_overlap.baseCount()>0)
		{
			info.coding_overlap = coding_overlap[0];

			if (coding_overlap.count()>1)
			{
				Log::warn("GSvar gap dialog: one gap split into to transcripts. Only using the first transcript!");
			}
		}

		//check if overlapping with perferred transcript
		BedFile pt_exon_regions;
		foreach(QByteArray gene, info.genes)
		{
			QString gene_approved = db.geneToApproved(gene, true);
			if (preferred_transcripts_.contains(gene_approved))
			{
				int gene_id = db.geneToApprovedID(gene);
				QList<Transcript> transcripts = db.transcripts(gene_id, Transcript::ENSEMBL, false);
				foreach(const Transcript& transcript, transcripts)
				{
					if (preferred_transcripts_[gene_approved].contains(transcript.name()))
					{
						pt_exon_regions.add(transcript.regions());
					}
				}
			}
		}
		pt_exon_regions.extend(5);
		if (pt_exon_regions.count()==0)
		{
			info.preferred_transcript = "";
		}
		else
		{
			info.preferred_transcript = pt_exon_regions.overlapsWith(info.line.chr(), info.line.start(), info.line.end()) ? "yes" : "no";
		}


		gaps_.append(info);
	}

	//show warning if non-coding transcripts had to be used
	if (!genes_noncoding.isEmpty())
	{
		QMessageBox::warning(this, "Non-coding transcrips were used for gaps!", "No coding transcript is defined for the following genes (for GRCh37):\n"+genes_noncoding.join(", ")+"\nFor these genes the longest *non-coding* transcript is used.\nPlease check gaps of these genes manually since they might be non-coding but shown as coding region +-5!");
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
		if (gap.isExonicSplicing()) size += " (" + QString::number(gap.coding_overlap.length()) + ")";
		ui->gaps->setItem(i, 1, createItem(size, false, true));

		//depth
		QString depth = QString::number(gap.avg_depth, 'f', 2);
		ui->gaps->setItem(i, 2, createItem(depth, gap.avg_depth<10, true));

		//genes
		ui->gaps->setItem(i, 3, createItem(gap.genes.join(", "), gap.genes.intersectsWith(genes)));

		//type
		ui->gaps->setItem(i, 4, createItem(gap.isExonicSplicing() ? "exonic/splicing" : "intronic/intergenic" , gap.isExonicSplicing()));

		//preferred transcripts
		ui->gaps->setItem(i, 5, createItem(gap.preferred_transcript, gap.preferred_transcript=="yes"));

		//validate
		ui->gaps->setItem(i, 6, new QTableWidgetItem());
		GapValidationLabel* label = new GapValidationLabel();
		ui->gaps->setCellWidget(i, 6, label);
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
	stream << "### Regionen mit eingeschraenkter diagnostischer Beurteilbarkeit (Ensembl+-5) ###\n";
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


void GapDialog::reportSection(QTextStream& stream, bool coding_only) const
{
	stream << "Sequenzabschnitte, die mittels Sanger-Sequenzierung analysiert wurden:\n";
	int closed_sanger = 0;
	for (int row=0; row<ui->gaps->rowCount(); ++row)
	{
		const GapInfo& gap = gaps_[row];

		if (coding_only && !gap.isExonicSplicing()) continue;

		if (state(row)==GapValidationLabel::VALIDATION)
		{
			stream << gap.asTsv(coding_only) << "\n";
			closed_sanger += coding_only ? gap.coding_overlap.length() : gap.line.length();
		}
	}
	stream << "\n";

	stream << "Sequenzabschnitte, die mittels visueller Inspektion analysiert wurden";
	if (!coding_only) stream << " (or intronic/intergenic)";
	stream << ":\n";
	int closed_manual = 0;
	for (int row=0; row<ui->gaps->rowCount(); ++row)
	{
		const GapInfo& gap = gaps_[row];

		if (coding_only && !gap.isExonicSplicing()) continue;

		if (state(row)==GapValidationLabel::NO_VALIDATION)
		{
			stream << gap.asTsv(coding_only) << "\n";
			closed_manual += coding_only ? gap.coding_overlap.length() : gap.line.length();
		}
	}
	stream << "\n";

	stream << "Sequenzabschnitte, die nicht analysiert wurden:\n";
	int closed_not = 0;
	for (int row=0; row<ui->gaps->rowCount(); ++row)
	{
		const GapInfo& gap = gaps_[row];

		if (coding_only && !gap.isExonicSplicing()) continue;

		if (state(row)==GapValidationLabel::CHECK)
		{
			stream << gap.asTsv(coding_only) << "\n";
			closed_not += coding_only ? gap.coding_overlap.length() : gap.line.length();
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
	return qobject_cast<GapValidationLabel*>(ui->gaps->cellWidget(row, 6))->state();
}
