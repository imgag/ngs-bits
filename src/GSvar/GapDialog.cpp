#include "GapDialog.h"
#include "GUIHelper.h"
#include "GSvarHelper.h"
#include "Statistics.h"
#include "GermlineReportGenerator.h"
#include "GlobalServiceProvider.h"
#include <QMessageBox>
#include <QMenu>

GapDialog::GapDialog(QWidget *parent, QString ps, QString bam_file, QString lowcov_file, const BedFile& roi, const GeneSet& genes)
	: QDialog(parent)
	, ui_()
	, init_timer_(this, true)
	, ps_(ps)
	, bam_(bam_file)
	, lowcov_file_(lowcov_file.trimmed())
	, roi_(roi)
	, genes_(genes)
	, ngsd_col_(7)
{
	ui_.setupUi(this);
	setWindowFlags(Qt::Window);
	setWindowTitle("Gaps of sample " + ps);

	connect(ui_.gaps, SIGNAL(itemDoubleClicked(QTableWidgetItem*)), this, SLOT(gapDoubleClicked(QTableWidgetItem*)));
	connect(ui_.gaps, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(gapsContextMenu(QPoint)));
	connect(ui_.f_gene, SIGNAL(textEdited(QString)), this, SLOT(updateFilters()));
	connect(ui_.f_type, SIGNAL(currentIndexChanged(int)), this, SLOT(updateFilters()));
}

void GapDialog::delayedInitialization()
{
	QString title = "Gap calculation";
	QStringList messages;
	messages << "Calculating gaps for target region now.";
	if (lowcov_file_.isEmpty())
	{
		messages << "";
		messages << "Gaps need to be calculated from BAM as there is no low-coverage file for this samples.";
		messages << "This may take a while if the target region is big!";
	}

	QMessageBox::information(this, title, messages.join("\n"));

	//calculate gaps
	QApplication::setOverrideCursor(Qt::BusyCursor);
	try
	{
		messages = calculteGapsAndInitGUI();
	}
	catch (Exception& e)
	{
		messages << e.message();
	}
	QApplication::restoreOverrideCursor();
	if (!messages.isEmpty())
	{
		QMessageBox::warning(this, title, messages.join("\n"));
	}

	//update NGSD column
	updateNGSDColumn();

	//check if there are already gaps for this sample
	QString ps_id = db_.processedSampleId(ps_);
	int gap_count = db_.getValue("SELECT count(id) FROM gaps WHERE processed_sample_id='" + ps_id +"'").toInt();
	if (gap_count>0)
	{
		QMessageBox::information(this, title, "There are already " + QString::number(gap_count) + " gaps for this sample in the NGSD!");
	}
}

QStringList GapDialog::calculteGapsAndInitGUI()
{
	//init
	QStringList output;
	int cutoff = 20;
    const QMap<QByteArray, QByteArrayList>& preferred_transcripts = GSvarHelper::preferredTranscripts();

	//calculate low-coverage regions
	BedFile low_cov;
	if (lowcov_file_.isEmpty())
	{
		low_cov = Statistics::lowCoverage(roi_, bam_, cutoff);
	}
	else
	{
		try
		{
			int sys_id = db_.processingSystemIdFromProcessedSample(ps_);
			BedFile sys_roi = GlobalServiceProvider::database().processingSystemRegions(sys_id);
			low_cov = GermlineReportGenerator::precalculatedGaps(lowcov_file_, roi_, cutoff, sys_roi);
		}
		catch(Exception e)
		{
			output << "Low-coverage statistics had to be re-calculated!";
			output << "Pre-calulated gap file could not be used because:";
			output << e.message();
			low_cov = Statistics::lowCoverage(roi_, bam_, cutoff);
		}
	}

	//show statistics
	QString roi_genes = "genes: " + QString::number(genes_.count()) + "<br>";
	QString roi_size = "bases: " + QString::number(roi_.baseCount()) + "<br>";
	QString gap_perc = "bases with depth&lt;" + QString::number(cutoff) + "x: " + QString::number(low_cov.baseCount()) + " (" + QString::number(100.0 * low_cov.baseCount() / roi_.baseCount(), 'f', 2) + "%)<br>" ;
	ui_.statistics->setText(roi_genes + roi_size + gap_perc);

	//calculate average coverage for gaps
	low_cov.clearAnnotations();
	Statistics::avgCoverage(low_cov, bam_, 1, false, true);

	//update data structure
	GeneSet genes_noncoding;
	for(int i=0; i<low_cov.count(); ++i)
	{
		GapInfo info;
		info.region = low_cov[i];
		bool ok = true;
		info.avg_depth = low_cov[i].annotations()[0].toDouble(&ok);
		if (!ok) output << "Could not convert average depth to decimal number for gap " + low_cov[i].toString(true);
		info.genes = db_.genesOverlappingByExon(info.region.chr(), info.region.start(), info.region.end(), 30);

		//use longest coding transcript(s) of Ensembl
		BedFile coding_overlap;
		foreach(QByteArray gene, info.genes)
		{
			int gene_id = db_.geneToApprovedID(gene);
			Transcript transcript = db_.longestCodingTranscript(gene_id, Transcript::ENSEMBL, true);
			if (!transcript.isValid())
			{
				genes_noncoding.insert(gene);
				transcript = db_.longestCodingTranscript(gene_id, Transcript::ENSEMBL, true, true);
			}
			coding_overlap.add(transcript.codingRegions());
		}
		coding_overlap.extend(5);
		BedFile current_gap;
		current_gap.append(info.region);
		coding_overlap.intersect(current_gap);
		if (coding_overlap.baseCount()>0)
		{
			info.coding_overlap = coding_overlap[0];

			if (coding_overlap.count()>1)
			{
				Log::warn("GSvar gap dialog: one gap split into to transcripts. Only using the first transcript!");
			}
		}

		//check if overlapping with preferred transcript
		BedFile pt_exon_regions;
		foreach(QByteArray gene, info.genes)
		{
			QByteArray gene_approved = db_.geneToApproved(gene, true);
            if (preferred_transcripts.contains(gene_approved))
			{
				int gene_id = db_.geneToApprovedID(gene);
				QList<Transcript> transcripts = db_.transcripts(gene_id, Transcript::ENSEMBL, false);
				foreach(const Transcript& transcript, transcripts)
				{
                    if (preferred_transcripts[gene_approved].contains(transcript.name()))
					{
						pt_exon_regions.add(transcript.regions());
					}
				}
			}
		}
		if (pt_exon_regions.count()==0) //no preferred transcripts defined
		{
			info.preferred_transcript = "";
		}
		else
		{
			//check for overlap with coding region
			pt_exon_regions.extend(5);
			pt_exon_regions.merge();
			if (pt_exon_regions.overlapsWith(info.region.chr(), info.region.start(), info.region.end()))
			{
				info.preferred_transcript = "yes";
			}
			else
			{
				//check for overlap with splice region
				BedFile pt_splice_regions = pt_exon_regions;
				pt_splice_regions.extend(15);
				pt_splice_regions.merge();
				pt_splice_regions.subtract(pt_exon_regions);

				if (pt_splice_regions.overlapsWith(info.region.chr(), info.region.start(), info.region.end()))
				{
					info.preferred_transcript = "yes (splice region)";
				}
				else
				{
					info.preferred_transcript = "no";
				}
			}
		}

		gaps_.append(info);
	}

	//show warning if non-coding transcripts had to be used
	if (!genes_noncoding.isEmpty())
	{
		output << "No coding transcript is defined for the following genes (for GRCh37):";
		output << genes_noncoding.join(", ");
		output << "For these genes the longest *non-coding* transcript is used.";
		output << "Please check gaps of these genes manually since they might be non-coding but shown as coding region +-5!";
	}

	//init GUI
	ui_.gaps->setRowCount(gaps_.count());
	for(int i=0; i<gaps_.count(); ++i)
	{
		//gap
		const GapInfo& gap = gaps_[i];
		QTableWidgetItem* item = GUIHelper::createTableItem(gap.region.toString(true));
		ui_.gaps->setItem(i, 0, item);

		//size
		QString size = QString::number(gap.region.length());
		if (gap.isExonicSplicing()) size += " (" + QString::number(gap.coding_overlap.length()) + ")";
		item = GUIHelper::createTableItem(size, Qt::AlignRight|Qt::AlignTop);
		ui_.gaps->setItem(i, 1, item);

		//depth
		QString depth = QString::number(gap.avg_depth, 'f', 2);
		item = GUIHelper::createTableItem(depth, Qt::AlignRight|Qt::AlignTop);
		if (gap.avg_depth<10) highlightItem(item);
		ui_.gaps->setItem(i, 2, item);

		//genes
		item = GUIHelper::createTableItem(gap.genes.join(", "));
		if (gap.genes.intersectsWith(genes_)) highlightItem(item);
		ui_.gaps->setItem(i, 3, item);

		//type
		item = GUIHelper::createTableItem(gap.isExonicSplicing() ? "exonic/splicing" : "intronic/intergenic");
		if (gap.isExonicSplicing()) highlightItem(item);
		ui_.gaps->setItem(i, 4, item);

		//preferred transcripts
		item = GUIHelper::createTableItem(gap.preferred_transcript);
		if (gap.preferred_transcript=="yes") highlightItem(item);
		ui_.gaps->setItem(i, 5, item);

		//suggested action
		QString action;
		if (gap.isExonicSplicing())
		{
			if (gap.avg_depth>=15)
			{
				action = "check in IGV";
			}
			else
			{
				action = "close by Sanger sequencing";
			}
		}
		item = GUIHelper::createTableItem(action);
		ui_.gaps->setItem(i, 6, item);

		//NGSD status
		item = GUIHelper::createTableItem("");
		ui_.gaps->setItem(i, ngsd_col_, item);
	}

	GUIHelper::resizeTableCells(ui_.gaps);

	return output;
}

void GapDialog::gapDoubleClicked(QTableWidgetItem* item)
{
	if (item==nullptr) return;

	QString region = ui_.gaps->item(item->row(), 0)->text();
	emit openRegionInIGV(region);
}

void GapDialog::updateFilters()
{
	//show all rows
	for (int i=0; i<ui_.gaps->rowCount(); ++i)
	{
		ui_.gaps->setRowHidden(i, false);
	}

	//filter by gene
	QString gene = ui_.f_gene->text().trimmed();
	if (gene!="")
	{
		for (int i=0; i<ui_.gaps->rowCount(); ++i)
		{
			bool hide = !ui_.gaps->item(i, 3)->text().contains(gene, Qt::CaseInsensitive);
			if (hide) ui_.gaps->setRowHidden(i, true);
		}
	}

	//filter by type
	QString type = ui_.f_type->currentText().trimmed();
	if (type!="")
	{
		for (int i=0; i<ui_.gaps->rowCount(); ++i)
		{
			bool hide = ui_.gaps->item(i, 4)->text()!=type;
			if (hide) ui_.gaps->setRowHidden(i, true);
		}
	}
}

void GapDialog::updateNGSDColumn()
{
	QString ps_id = db_.processedSampleId(ps_);

	for(int i=0; i<gaps_.count(); ++i)
	{
		QTableWidgetItem* item = ui_.gaps->item(i, ngsd_col_);
		item->setText("");
		item->setData(Qt::UserRole, -1);
		item->setIcon(QIcon());

		int gap_id = db_.gapId(ps_id, gaps_[i].region.chr(), gaps_[i].region.start(), gaps_[i].region.end());
		if (gap_id!=-1)
		{
			QString status = db_.getValue("SELECT status FROM gaps WHERE id='" + QString::number(gap_id) + "'").toString();
			item->setText(status);
			item->setData(Qt::UserRole, gap_id);

			if (status=="checked visually" || status=="closed")
			{
				item->setIcon(QIcon(":/Icons/Ok.png"));
			}
		}
	}
}

void GapDialog::gapsContextMenu(QPoint pos)
{
	//determine row and item
	if  (ui_.gaps->selectedItems().count()==0) return;
	int row = ui_.gaps->selectedItems()[0]->row();
	QTableWidgetItem* item = ui_.gaps->item(row, ngsd_col_);
	if (item==nullptr) return;

	//set up menu
	QMenu menu;
	QAction* action_visual = menu.addAction("checked visually");
	QAction* action_close = menu.addAction("close by Sanger sequencing");
	QAction* action_cancel = menu.addAction("cancel");
	QString status = item->text().trimmed();
	action_visual->setEnabled(status=="" || status=="to close" || status=="canceled");
	action_close->setEnabled(status=="" || status=="canceled" || status=="checked visually");
	action_cancel->setEnabled(status=="to close" || status=="checked visually");

	//show menu
	QAction* action = menu.exec(ui_.gaps->viewport()->mapToGlobal(pos));
	if (action==nullptr) return;

	//update NGSD
	QString new_status;
	if (action==action_visual) new_status = "checked visually";
	if (action==action_close) new_status = "to close";
	if (action==action_cancel) new_status = "canceled";

	int gap_id = item->data(Qt::UserRole).toInt();
	if (gap_id==-1)
	{
		QString ps_id = db_.processedSampleId(ps_);
		db_.addGap(ps_id, gaps_[row].region.chr(), gaps_[row].region.start(), gaps_[row].region.end(), new_status);
	}
	else
	{
		db_.updateGapStatus(gap_id, new_status);
	}

	//update GUI
	updateNGSDColumn();
}

void GapDialog::highlightItem(QTableWidgetItem* item)
{
	QFont font = item->font();
	font.setBold(true);
	item->setFont(font);
}
