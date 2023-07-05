#include "GapDialog.h"
#include "GUIHelper.h"
#include "GSvarHelper.h"
#include "GermlineReportGenerator.h"
#include "GlobalServiceProvider.h"
#include "Log.h"
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
	connect(ui_.copy_btn, SIGNAL(clicked(bool)), this, SLOT(copyToClipboard()));
	connect(ui_.gap_btn, SIGNAL(clicked(bool)), this, SLOT(calculteGaps()));
}

void GapDialog::delayedInitialization()
{
	QString title = "Gap calculation";
	if (lowcov_file_.isEmpty())
	{
		QStringList messages;
		messages << "Gaps need to be calculated from BAM as there is no low-coverage file for this samples.";
		messages << "This may take a while if the target region is big!";
		QMessageBox::information(this, title, messages.join("\n"));
	}

	//check if there are already gaps for this sample
	QString ps_id = db_.processedSampleId(ps_);
	int gap_count = db_.getValue("SELECT count(id) FROM gaps WHERE processed_sample_id='" + ps_id +"'").toInt();
	if (gap_count>0)
	{
		QMessageBox::information(this, title, "There are already " + QString::number(gap_count) + " gaps for this sample in the NGSD!");
	}
}

void GapDialog::calculteGaps()
{
	//clear GUI
	ui_.f_gene->clear();
	ui_.f_type->setCurrentText(0);

	//init
	QStringList output;
	int cutoff = 20;
	const QMap<QByteArray, QByteArrayList>& preferred_transcripts = GSvarHelper::preferredTranscripts();
	gaps_.clear();

	//calculation
	QApplication::setOverrideCursor(Qt::BusyCursor);

	//determine ROI
	BedFile roi;
	if (ui_.reduce_exon_splicing->isChecked())
	{
		GeneSet invalid_genes;
		foreach(const QByteArray& gene, genes_)
		{
			int gene_id = db_.geneId(gene);
			if (gene_id==-1)
			{
				invalid_genes << gene;
				continue;
			}

			TranscriptList transcripts = db_.releventTranscripts(gene_id);
			foreach(const Transcript& transcript, transcripts)
			{
				if (transcript.isCoding())
				{
					roi.add(transcript.codingRegions());
				}
				else
				{
					roi.add(transcript.regions());
				}
			}
		}
		roi.extend(20);
		roi.merge();
		roi.sort();

		if (!invalid_genes.isEmpty())
		{
			output << "Invalid gene names could not be converted to exons: " + invalid_genes.join(", ");
		}
	}
	else
	{
		roi = roi_;
	}

	try
	{
		//calculate low-coverage regions
		BedFile low_cov;
		if (lowcov_file_.isEmpty())
		{
			low_cov = GlobalServiceProvider::statistics().lowCoverage(roi, bam_, cutoff);
		}
		else
		{
			try
			{
				//load gaps file
				BedFile gaps;
				gaps.load(lowcov_file_, false, false);

				//load system ROI
				int sys_id = db_.processingSystemIdFromProcessedSample(ps_);
				BedFile sys_roi = GlobalServiceProvider::database().processingSystemRegions(sys_id, false);

				low_cov = GermlineReportGenerator::precalculatedGaps(gaps, roi, cutoff, sys_roi);
			}
			catch(Exception e)
			{
				output << "Low-coverage statistics had to be re-calculated!";
				output << "Pre-calculated gap file could not be used because:";
				output << e.message();
				low_cov = GlobalServiceProvider::statistics().lowCoverage(roi, bam_, cutoff);
			}
		}

		//show statistics
		QString roi_genes = "genes: " + QString::number(genes_.count()) + "<br>";
		QString roi_size = "bases: " + QString::number(roi.baseCount()) + "<br>";
		QString gap_perc = "bases with depth&lt;" + QString::number(cutoff) + "x: " + QString::number(low_cov.baseCount()) + " (" + QString::number(100.0 * low_cov.baseCount() / roi.baseCount(), 'f', 2) + "%)<br>" ;
		ui_.statistics->setText(roi_genes + roi_size + gap_perc);

		//calculate average coverage for gaps
		low_cov.clearAnnotations();
		int threads = Settings::integer("threads");
		GlobalServiceProvider::statistics().avgCoverage(low_cov, bam_, threads);

		//update data structure
		for(int i=0; i<low_cov.count(); ++i)
		{
			GapInfo info;
			info.region = low_cov[i];
			bool ok = true;
			info.avg_depth = low_cov[i].annotations()[0].toDouble(&ok);
			if (!ok) output << "Could not convert average depth to decimal number for gap " + low_cov[i].toString(true);
			info.genes = db_.genesOverlappingByExon(info.region.chr(), info.region.start(), info.region.end(), 30);

			//use relevant transcripts
			BedFile coding_overlap;
			foreach(QByteArray gene, info.genes)
			{
				int gene_id = db_.geneId(gene);
				if (gene_id==-1) continue;
				TranscriptList transcripts = db_.releventTranscripts(gene_id);
				foreach(const Transcript& transcript, transcripts)
				{
					coding_overlap.add(transcript.isCoding() ? transcript.codingRegions() : transcript.regions());
				}
			}
			coding_overlap.extend(20);
			coding_overlap.merge();
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
					int gene_id = db_.geneId(gene);
					TranscriptList transcripts = db_.transcripts(gene_id, Transcript::ENSEMBL, false);
					foreach(const Transcript& transcript, transcripts)
					{
						if (transcript.isPreferredTranscript())
						{
							pt_exon_regions.add(transcript.regions());
						}
					}
				}
			}
			pt_exon_regions.merge();

			if (pt_exon_regions.count()==0) //no preferred transcripts defined
			{
				info.preferred_transcript = "";
			}
			else
			{
				//check for overlap with coding region
				if (pt_exon_regions.overlapsWith(info.region.chr(), info.region.start(), info.region.end()))
				{
					info.preferred_transcript = "yes";
				}
				else
				{
					//check for overlap with splice region
					BedFile pt_splice_regions = pt_exon_regions;
					pt_splice_regions.extend(20);
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

<<<<<<< .mine
		//NGSD status
		item = GUIHelper::createTableItem(QString());
		ui_.gaps->setItem(i, ngsd_col_, item);

=======
		GUIHelper::resizeTableCells(ui_.gaps);

		//update NGSD column
		updateNGSDColumn();
>>>>>>> .theirs
	}
	catch (Exception& e)
	{
		output << e.message();
	}

	QApplication::restoreOverrideCursor();

	if (!output.isEmpty())
	{
		QMessageBox::warning(this, "Gap calculation", output.join("\n"));
	}
}

void GapDialog::gapDoubleClicked(QTableWidgetItem* item)
{
	if (item==nullptr) return;

	QString region = ui_.gaps->item(item->row(), 0)->text();
	GlobalServiceProvider::gotoInIGV(region, true);
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
	int ps_id = db_.processedSampleId(ps_).toInt();

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
		int ps_id = db_.processedSampleId(ps_).toInt();
		db_.addGap(ps_id, gaps_[row].region.chr(), gaps_[row].region.start(), gaps_[row].region.end(), new_status);
	}
	else
	{
		db_.updateGapStatus(gap_id, new_status);
	}

	//update GUI
	updateNGSDColumn();
}

void GapDialog::copyToClipboard()
{
	GUIHelper::copyToClipboard(ui_.gaps);
}

void GapDialog::highlightItem(QTableWidgetItem* item)
{
	QFont font = item->font();
	font.setBold(true);
	item->setFont(font);
}
