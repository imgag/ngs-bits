#include "GeneSelectorDialog.h"
#include "ui_GeneSelectorDialog.h"
#include "ReportWorker.h"
#include "Helper.h"
#include "NGSD.h"
#include "NGSHelper.h"
#include "GUIHelper.h"
#include <QDebug>
#include <QFileInfo>
#include <QMessageBox>
#include "GlobalServiceProvider.h"

GeneSelectorDialog::GeneSelectorDialog(QString sample_name, QWidget* parent)
	: QDialog(parent)
	, ui(new Ui::GeneSelectorDialog)	
	, sample_name_(sample_name)
{
	ui->setupUi(this);
	ui->splitter->setStretchFactor(0, 1);
	ui->splitter->setStretchFactor(1, 3);
	GUIHelper::styleSplitter(ui->splitter);

	// create tool tips for details header cells
	ui->details->horizontalHeaderItem(5)->setToolTip("<html><head/><body><p>Lists number of (high-quality) CNVs of the current sample which overlaps the transcript region of the selected genes.</p></body></html>");
	ui->details->horizontalHeaderItem(6)->setToolTip("<html><head/><body><p>Lists number of low-quality CNVs (marked as failing by CnvHunter or log-likelihood &le; 20 in case of ClinCNV) of the current sample which overlaps the transcript region of the selected gene as 'bad qc' and the number of transcript regions which are not present in the copy-number seg file as 'not covered' </p></body></html>");

	connect(ui->update_btn, SIGNAL(pressed()), this, SLOT(updateGeneTable()));
	connect(ui->details, SIGNAL(cellChanged(int,int)), this, SLOT(geneTableItemChanged(int, int)));
	connect(ui->details, SIGNAL(itemDoubleClicked(QTableWidgetItem*)), this, SLOT(geneDoubleClicked(QTableWidgetItem*)));
}

GeneSelectorDialog::~GeneSelectorDialog()
{
	delete ui;
}

void GeneSelectorDialog::updateGeneTable()
{
	//clear details
	ui->details->clearContents();

	//convert input to gene list
	GeneSet genes = GeneSet::createFromText(ui->genes->toPlainText().toLatin1());
	if (genes.isEmpty()) return;

	//set cursor
	QApplication::setOverrideCursor(Qt::BusyCursor);
	ui->details->blockSignals(true); //otherwise itemChanged is emitted

	//check for CN calling results
	QStringList seg_files = GlobalServiceProvider::fileLocationProvider().getCnvCoverageFiles(false).filterById(sample_name_).asStringList();
	QStringList tsv_files = GlobalServiceProvider::fileLocationProvider().getCopyNumberCallFiles(false).filterById(sample_name_).asStringList();
	bool cnv_data_found = seg_files.count()==1 && tsv_files.size()==1;
	if (!cnv_data_found)
	{
		QMessageBox::warning(this, "CNV data not found", "CNV files not found not in sample folder.\nSkipping CNV statistics!\n\nThis should only happen if CNV calling was not possible for the sample!");
	}

	//load CNA results
	BedFile cnv_calls;
	BedFile cnv_regions_skipped;
	if (cnv_data_found)
	{
		//load calls
		cnv_calls.load(tsv_files[0]);
		cnv_calls.merge();

		//load skipped regions from SEG file
		auto f = Helper::openFileForReading(seg_files[0]);
		while(!f->atEnd())
		{
			QByteArray line = f->readLine();

			//skip empty and comment lines
			if (line.trimmed().isEmpty() || line.startsWith('#')) continue;
			if (line.startsWith("ID\t")) continue;

			QList<QByteArray> parts = line.split('\t');
			if (parts.count()<5) THROW(FileParseException, "SEG file line invalid - less than 5 few tab-separated parts: " + line);

			//determine if region QC failed
			//CnvHunter: log2-ratio = "QC failed"
			//ClinCNV: variance = "LowRawCoverage", "TooShortOrNA", "GCnormFailed", ...
			bool ok = true;
			parts[4].toDouble(&ok);
			if (!ok)
			{
				//parse content
				Chromosome chr(parts[1]);
				int start = Helper::toInt(parts[2], "SEG start position", line)+1;
				int end = Helper::toInt(parts[3], "SEG end position", line)+1;
				cnv_regions_skipped.append(BedLine(chr, start, end));
			}
		}
		cnv_regions_skipped.merge();
	}

	//load low-coverage file for processing system
	BedFile sys_gaps;
	QStringList lowcov_files = GlobalServiceProvider::fileLocationProvider().getLowCoverageFiles(false).filterById(sample_name_).asStringList();
	if(lowcov_files.count()==1)
	{
		sys_gaps.load(lowcov_files[0]);
	}
	else
	{
		QMessageBox::warning(this, "Gaps not found", "Low-coverage BED file not found in sample folder.\nSkipping gap statistics!\n\nThis is normal for WGS samples, but should not happen for panel/exome!");
	}

	//load processing system target region

	NGSD db;
	BedFile sys_roi = GlobalServiceProvider::database().processingSystemRegions(db.processingSystemIdFromProcessedSample(sample_name_));
	if (sys_roi.isEmpty())
	{
		updateError("Gene selection error", "Processing system target region file not found for sample '" + sample_name_ +  "'");
		return;
	}

	//display genes
	ui->details->setRowCount(genes.count());
	for (int r=0; r<genes.count(); ++r)
	{
		//convert gene to approved symbol
		QByteArray gene = genes[r];
		int gene_id = db.geneToApprovedID(gene);
		if(gene_id==-1)
		{
			updateError("Gene selection error", "Gene symbol '" + gene + "' is not an approved symbol!");
			return;
		}
		gene = db.geneSymbol(gene_id);
		setGeneTableItem(r, 0, gene, Qt::AlignLeft, Qt::ItemIsUserCheckable|Qt::ItemIsEnabled);

		//transcript
		Transcript transcript = db.longestCodingTranscript(gene_id, Transcript::CCDS, true);
		BedFile region = transcript.codingRegions();
		setGeneTableItem(r, 1, transcript.name() + " (" + QString::number(region.count()) + " exons)");

		//size
		region.extend(5);
		region.merge();
		long long bases = region.baseCount();
		setGeneTableItem(r, 2, QString::number(bases), Qt::AlignRight);

		//calculate gaps inside target region
		if (sys_gaps.count()==0)
		{
			setGeneTableItem(r, 3, "n/a", Qt::AlignRight);
			setGeneTableItem(r, 4, "n/a", Qt::AlignRight);
		}
		else
		{
			BedFile gaps = sys_gaps;
			gaps.intersect(region);

			//add target region bases not covered by processing system target file
			BedFile uncovered(region);
			uncovered.subtract(sys_roi);
			gaps.add(uncovered);
			gaps.merge();

			//output (absolute and percentage)
			long long gap_bases = gaps.baseCount();
			setGeneTableItem(r, 3, QString::number(gap_bases), Qt::AlignRight);
			setGeneTableItem(r, 4, QString::number(100.0 * gap_bases / bases, 'f', 2), Qt::AlignRight);
		}

		//CNVs detected and CNV region fails
		if (cnv_data_found)
		{
			BedFile tmp = region;
			tmp.overlapping(cnv_calls);
			setGeneTableItem(r, 5, QString::number(tmp.count()), Qt::AlignRight, Qt::ItemIsUserCheckable|Qt::ItemIsEnabled);

			tmp = region;
			tmp.overlapping(cnv_regions_skipped);
			setGeneTableItem(r, 6, QString::number(tmp.count()), Qt::AlignRight);
		}
		else
		{
			setGeneTableItem(r, 5, "n/a", Qt::AlignRight);
			setGeneTableItem(r, 6, "n/a", Qt::AlignRight);
		}
	}

	//resize
	GUIHelper::resizeTableCells(ui->details);

	//reset cursor
	QApplication::restoreOverrideCursor();
	ui->details->blockSignals(false);
	updateSelectedGenesStatistics();
}

void GeneSelectorDialog::geneTableItemChanged(int /*row*/, int col)
{
	if (col==0 || col==5)
	{
		updateSelectedGenesStatistics();
	}
}

void GeneSelectorDialog::geneDoubleClicked(QTableWidgetItem* item)
{
	if (item==nullptr) return;

	emit openRegionInIGV(ui->details->item(item->row(), 0)->text());
}

QString GeneSelectorDialog::report()
{
	QString output;
	QTextStream stream(&output);

	//header
	stream << "Gene selection report\n";
	stream << "\n";
	stream << "Sample: " << sample_name_ << "\n";
	NGSD db;
	ProcessingSystemData system_data = db.getProcessingSystemData(db.processingSystemIdFromProcessedSample(sample_name_));
	stream << "Target region: " << system_data.name << "\n";
	stream << "\n";

	//selected genes
	stream << "Genes selected for variant analysis:\n";
	stream << "#gene\ttranscript\tsize\tgaps\n";
	long sum = 0;
	long gaps = 0;
	for (int r=0; r<ui->details->rowCount(); ++r)
	{
		if (ui->details->item(r, 0)->checkState() == Qt::Checked)
		{
			stream << ui->details->item(r, 0)->text() << "\t" << ui->details->item(r, 1)->text() << "\t" << ui->details->item(r, 2)->text() << "\t" << ui->details->item(r, 3)->text() << "\n";
			sum += ui->details->item(r, 2)->text().toInt();
			gaps += ui->details->item(r, 3)->text().toInt();
		}
	}

	//selected genes statistics
	stream << "Bases  : " << QString::number(sum) << "\n";
	stream << "Gaps   : " << QString::number(gaps) << "\n";
	stream << "Overall: " << QString::number(sum-gaps) << "\n";
	stream << "\n";


	//selected genes for CNA
	stream << "Genes selected for CNV analysis:\n";
	stream << "#gene\ttranscript\tCNVs detected\tCNV regions missing\n";;
	for (int r=0; r<ui->details->rowCount(); ++r)
	{
		if (ui->details->item(r, 5)->checkState() == Qt::Checked)
		{
			stream << ui->details->item(r, 0)->text() << "\t" << ui->details->item(r, 1)->text() << "\t" << ui->details->item(r, 5)->text() << "\t" << ui->details->item(r, 6)->text() << "\n";
		}
	}

	stream.flush();
	return  output;
}

GeneSet GeneSelectorDialog::genesForVariants()
{
	GeneSet output;

	for (int r=0; r<ui->details->rowCount(); ++r)
	{
		if (ui->details->item(r, 0)->checkState() == Qt::Checked)
		{
			output << ui->details->item(r, 0)->text().toLatin1();
		}
	}

	return output;
}

void GeneSelectorDialog::setGeneTableItem(int row, int col, QString text, int alignment, Qt::ItemFlags flags)
{
	auto item = new QTableWidgetItem(text);

	item->setFlags(flags);
	if (flags & Qt::ItemIsUserCheckable)
	{
		item->setCheckState(Qt::Unchecked);
	}

	item->setTextAlignment(Qt::AlignVCenter|alignment);

	ui->details->setItem(row, col, item);
}

void GeneSelectorDialog::updateSelectedGenesStatistics()
{
	int sel_var = 0;
	long sum = 0;
	long gaps = 0;
	int sel_cnv = 0;
	for (int r=0; r<ui->details->rowCount(); ++r)
	{
		if (ui->details->item(r, 0)->checkState() == Qt::Checked)
		{
			++sel_var;
			sum += ui->details->item(r, 2)->text().toInt();
			gaps += ui->details->item(r, 3)->text().toInt();
		}
		if (ui->details->item(r, 5)->checkState() == Qt::Checked)
		{
			++sel_cnv;
		}
	}
	ui->selection_details->setText("Gene for variant analysis: " + QString::number(sel_var) + " - base sum: " + QString::number(sum) + " - gap sum: " + QString::number(gaps) + "  //  Genes for CNA: " + QString::number(sel_cnv));
}

void GeneSelectorDialog::updateError(QString title, QString text)
{
	//show warning
	QMessageBox::warning(this, title, text);

	//reset content
	ui->details->clearContents();
	ui->details->setRowCount(0);

	//clear cursor/block
	QApplication::restoreOverrideCursor();
	ui->details->blockSignals(false);
}
