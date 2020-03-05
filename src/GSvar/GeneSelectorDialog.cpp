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

GeneSelectorDialog::GeneSelectorDialog(QString sample_folder, QString sample_name, QWidget* parent)
	: QDialog(parent)
	, ui(new Ui::GeneSelectorDialog)
	, sample_folder_(sample_folder)
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
	QApplication::setOverrideCursor(QCursor(Qt::BusyCursor));
	ui->details->blockSignals(true); //otherwise itemChanged is emitted

	//check for CNA results
	//check for ClinCNV CN calling:
	bool clincnv = false;
	bool cna_result_present = false;
	QStringList seg_files = Helper::findFiles(sample_folder_, "*_cnvs_clincnv.seg", false);
	QStringList tsv_files = Helper::findFiles(sample_folder_, "*_cnvs_clincnv.tsv", false);
	if (seg_files.count() > 0 && tsv_files.size() > 0)
	{
		clincnv = true;
		cna_result_present = true;
	}

	//if no ClinCNV files found check for CnvHunter
	if (!clincnv)
	{
		seg_files = Helper::findFiles(sample_folder_, "*_cnvs.seg", false);
		cna_result_present = (seg_files.count()==1);
	}




	//load CNA results
	BedFile cnvhunter_seg_file;
	BedFile clincnv_seg_file;
	BedFile clincnv_tsv_file;
	if (cna_result_present)
	{
		if (clincnv)
		{
			//ClinCNV
			// read SEG file
			auto f = Helper::openFileForReading(seg_files[0]);
			bool header_line_parsed = false;
			while(!f->atEnd())
			{
				QByteArray line = f->readLine();

				//skip empty and comment lines
				if (line.isEmpty() || line.startsWith('#')) continue;

				if (!header_line_parsed)
				{
					// skip first non-comment line (-> header)
					header_line_parsed = true;
					continue;
				}

				//parse content
				QList<QByteArray> parts = line.split('\t');
				if (parts.count()<6) THROW(FileParseException, "SEG file line invalid: " + line);
				Chromosome chr(parts[1]);
				int start = Helper::toInt(parts[2], "SEG start position", line);
				int end = Helper::toInt(parts[3], "SEG end position", line);
				clincnv_seg_file.append(BedLine(chr, start, end));
			}
			clincnv_seg_file.merge();
			clincnv_seg_file.sort();

			// read TSV CNV file
			clincnv_tsv_file.load(tsv_files[0]);
		}
		else
		{
			//CnvHunter
			auto f = Helper::openFileForReading(seg_files[0]);
			while(!f->atEnd())
			{
				QByteArray line = f->readLine();

				//skip headers
				if (line.isEmpty() || line[0]!='\t')
				{
					continue;
				}

				//parse content
				QList<QByteArray> parts = line.split('\t');
				if (parts.count()<6) THROW(FileParseException, "SEG file line invalid: " + line);
				Chromosome chr(parts[1]);
				int start = Helper::toInt(parts[2], "SEG start position", line);
				int end = Helper::toInt(parts[3], "SEG end position", line);
				cnvhunter_seg_file.append(BedLine(chr, start, end, QList<QByteArray>() << parts[5]));
			}
		}
	}

	//load low-coverage file for processing system
	QStringList files = Helper::findFiles(sample_folder_, "*_lowcov.bed", false);
	if(files.count()!=1)
	{
		updateError("Gene selection error", "Low-coverage BED file not found in " + sample_folder_);
		return;
	}
	BedFile sys_gaps;
	sys_gaps.load(files[0]);

	//load processing system target region
	NGSD db;
	ProcessingSystemData system_data = db.getProcessingSystemData(db.processingSystemIdFromProcessedSample(sample_name_), true);
	if (system_data.target_file=="")
	{

		updateError("Gene selection error", "Processing system target region BED file not found for sample '" + sample_name_ +  "'");
		return;
	}
	BedFile sys_roi;
	sys_roi.load(system_data.target_file);

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

		//cnvs + cnv gaps
		if (!cna_result_present)
		{
			setGeneTableItem(r, 5, "n/a", Qt::AlignRight);
			setGeneTableItem(r, 6, "n/a", Qt::AlignRight);
		}
		else
		{
			BedFile cnv_data;
			if (clincnv)
			{
				cnv_data = clincnv_tsv_file;
			}
			else
			{
				cnv_data = cnvhunter_seg_file;
			}
			cnv_data.overlapping(region);
			cnv_data.sort();
			int cnv_del = 0;
			int cnv_dup = 0;
			int cnv_bad_qc = 0;
			for(int i=0; i<cnv_data.count(); ++i)
			{
				// count CNVs with low log-likelihood In case of ClinCNV
				if (clincnv)
				{
					if (Helper::toDouble(cnv_data[i].annotations()[1], "CNV log-likelihood") <= 20.00)
					{
						++cnv_bad_qc;
						continue;
					}
				}

				QString cn = cnv_data[i].annotations()[0];
				bool ok = false;
				int cn_num = cn.toInt(&ok);
				if (!ok)
				{
					++cnv_bad_qc;
				}
				else if(cn_num<2)
				{
					++cnv_del;
				}
				else if(cn_num>2)
				{
					++cnv_dup;
				}
			}
			QStringList parts;
			if (cnv_del) parts << QString::number(cnv_del) + " del";
			if (cnv_dup) parts << QString::number(cnv_dup) + " dup";
			setGeneTableItem(r, 5, parts.join(", "), Qt::AlignRight, Qt::ItemIsUserCheckable|Qt::ItemIsEnabled);

			parts.clear();
			if (cnv_bad_qc) parts << QString::number(cnv_bad_qc) + " bad qc";
			int n_cnv_gaps;
			BedFile region_covered = region;

			if (clincnv)
			{
				region_covered.overlapping(clincnv_seg_file);
			}
			else
			{
				region_covered.overlapping(cnv_data);
			}
			n_cnv_gaps = region.count() - region_covered.count();
			if (n_cnv_gaps) parts << QString::number(n_cnv_gaps) + " not covered";
			setGeneTableItem(r, 6, parts.join(", "), Qt::AlignRight);
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
	ProcessingSystemData system_data = db.getProcessingSystemData(db.processingSystemIdFromProcessedSample(sample_name_), true);
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
