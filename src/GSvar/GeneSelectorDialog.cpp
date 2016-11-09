#include "GeneSelectorDialog.h"
#include "ui_GeneSelectorDialog.h"
#include "ReportWorker.h"
#include "Helper.h"
#include "NGSD.h"
#include <QDebug>
#include <QFileInfo>
#include <QMessageBox>

GeneSelectorDialog::GeneSelectorDialog(QString bam_file, QWidget* parent)
	: QDialog(parent)
	, ui(new Ui::GeneSelectorDialog)
	, bam_file_(bam_file)
{
	ui->setupUi(this);
	ui->pheno->setDetailsWidget(ui->pheno_details);

	connect(ui->pheno, SIGNAL(phenotypeActivated(QString)), this, SLOT(updateGeneTable(QString)));
	connect(ui->genes, SIGNAL(cellChanged(int,int)), this, SLOT(geneTableItemChanged(int, int)));
	connect(ui->genes, SIGNAL(itemDoubleClicked(QTableWidgetItem*)), this, SLOT(geneDoubleClicked(QTableWidgetItem*)));
}

GeneSelectorDialog::~GeneSelectorDialog()
{
	delete ui;
}

void GeneSelectorDialog::updateGeneTable(QString phenotype)
{
	//clear genes
	ui->genes->clearContents();
	if (phenotype=="") return;

	//set cursor
	QApplication::setOverrideCursor(QCursor(Qt::BusyCursor));
	ui->genes->blockSignals(true); //otherwise itemChanged is emitted

	//get gene list
	NGSD db;
	QStringList genes = db.phenotypeToGenes(phenotype, true);

	//check for CNA results
	QString folder = QFileInfo(bam_file_).absolutePath();
	QStringList files = Helper::findFiles(folder, "*_cnvs.seg", false);
	bool cna_result_present = (files.count()==1);

	//load CNA results
	BedFile cna_results;
	if (cna_result_present)
	{
		auto f = Helper::openFileForReading(files[0]);
		while(!f->atEnd())
		{
			QString line = f->readLine();

			//skip headers
			if (line.isEmpty() || line[0]!='\t')
			{
				continue;
			}

			//parse content
			QStringList parts = line.split('\t');
			if (parts.count()<6) THROW(FileParseException, "SEG file line invalid: " + line);
			Chromosome chr(parts[1]);
			int start = Helper::toInt(parts[2], "SEG start position", line);
			int end = Helper::toInt(parts[3], "SEG end position", line);
			cna_results.append(BedLine(chr, start, end, QStringList() << parts[5]));
		}
	}

	//load low-coverage file for processing system
	files = Helper::findFiles(folder, "*_lowcov.bed", false);
	if(files.count()!=1)
	{
		QMessageBox::warning(this, "Gene selection error", "Low-coverage BED file not found in " + folder);
		return;
	}
	BedFile sys_gaps;
	sys_gaps.load(files[0]);

	//load processing system target region
	QString sys_file = db.getProcessingSystem(bam_file_, NGSD::FILE);
	if (sys_file=="")
	{
		QMessageBox::warning(this, "Gene selection error", "Low-coverage BED file not found in " + folder);
		return;
	}
	BedFile sys_roi;
	sys_roi.load(sys_file);

	//display genes
	ui->genes->setRowCount(genes.count());
	for (int r=0; r<genes.count(); ++r)
	{
		//gene
		QString gene = genes[r];
		setGeneTableItem(r, 0, gene, Qt::AlignLeft, Qt::ItemIsUserCheckable|Qt::ItemIsEnabled);

		//transcript
		int gene_id = db.geneToApprovedID(gene);
		QString transcript;
		BedFile region;
		Chromosome chr;
		db.longestCodingTranscript(gene_id, "ccds", transcript, region, chr);
		setGeneTableItem(r, 1, transcript + " (" + QString::number(region.count()) + " exons)");

		//size
		region.extend(5);
		region.merge();
		int bases = region.baseCount();
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
		int gap_bases = gaps.baseCount();
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
			BedFile cnv_data = cna_results;
			cnv_data.overlapping(region);
			cnv_data.sort();
			int cnv_del = 0;
			int cnv_dup = 0;
			int cnv_bad_qc = 0;
			for(int i=0; i<cnv_data.count(); ++i)
			{
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

			BedFile region_covered = region;
			region_covered.overlapping(cnv_data);
			int cnv_gaps = region.count() - region_covered.count();
			parts.clear();
			if (cnv_bad_qc) parts << QString::number(cnv_bad_qc) + " bad qc";
			if (cnv_gaps) parts << QString::number(cnv_gaps) + " not covered";

			setGeneTableItem(r, 6, parts.join(", "), Qt::AlignRight);
		}
	}

	//resize
	ui->genes->resizeColumnsToContents();
	ui->genes->resizeRowsToContents();

	//reset cursor
	QApplication::restoreOverrideCursor();
	ui->genes->blockSignals(false);
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

	emit openRegionInIGV(ui->genes->item(item->row(), 0)->text());
}

QString GeneSelectorDialog::report()
{
	QString output;
	QTextStream stream(&output);

	//header
	stream << "Gene selection report\n";
	stream << "\n";
	stream << "Sample: " << QFileInfo(bam_file_).fileName().replace(".bam", "") << "\n";
	NGSD db;
	stream << "Target region: " << db.getProcessingSystem(bam_file_, NGSD::LONG) << "\n";
	stream << "\n";

	//selected genes
	stream << "Genes selected:\n";
	stream << "#gene\ttranscript\tsize\tgaps\n";
	long sum = 0;
	long gaps = 0;
	for (int r=0; r<ui->genes->rowCount(); ++r)
	{
		if (ui->genes->item(r, 0)->checkState() == Qt::Checked)
		{
			stream << ui->genes->item(r, 0)->text() << "\t" << ui->genes->item(r, 1)->text() << "\t" << ui->genes->item(r, 2)->text() << "\t" << ui->genes->item(r, 3)->text() << "\n";
			sum += ui->genes->item(r, 2)->text().toInt();
			gaps += ui->genes->item(r, 3)->text().toInt();
		}
	}

	//selected genes statistics
	stream << "Bases  : " << QString::number(sum) << "\n";
	stream << "Gaps   : " << QString::number(gaps) << "\n";
	stream << "Overall: " << QString::number(sum-gaps) << "\n";
	stream << "\n";


	//selected genes for CNA
	stream << "Genes selected:\n";
	stream << "#gene\ttranscript\tCNVs detected\tCNV regions missing\n";;
	for (int r=0; r<ui->genes->rowCount(); ++r)
	{
		if (ui->genes->item(r, 5)->checkState() == Qt::Checked)
		{
			stream << ui->genes->item(r, 0)->text() << "\t" << ui->genes->item(r, 1)->text() << "\t" << ui->genes->item(r, 5)->text() << "\t" << ui->genes->item(r, 6)->text() << "\n";
		}
	}

	stream.flush();
	return  output;
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

	ui->genes->setItem(row, col, item);
}

void GeneSelectorDialog::updateSelectedGenesStatistics()
{
	int sel_var = 0;
	long sum = 0;
	long gaps = 0;
	int sel_cnv = 0;
	for (int r=0; r<ui->genes->rowCount(); ++r)
	{
		if (ui->genes->item(r, 0)->checkState() == Qt::Checked)
		{
			++sel_var;
			sum += ui->genes->item(r, 2)->text().toInt();
			gaps += ui->genes->item(r, 3)->text().toInt();
		}
		if (ui->genes->item(r, 5)->checkState() == Qt::Checked)
		{
			++sel_cnv;
		}
	}
	ui->selection_details->setText("Gene for variant analysis: " + QString::number(sel_var) + " - base sum: " + QString::number(sum) + " - gap sum: " + QString::number(gaps) + "  //  Genes for CNA: " + QString::number(sel_cnv));
}
