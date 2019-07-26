#include "CnvWidget.h"
#include "ui_CnvWidget.h"
#include "Helper.h"
#include "Exceptions.h"
#include "GUIHelper.h"
#include "VariantDetailsDockWidget.h"
#include "NGSD.h"
#include "Settings.h"
#include <QMessageBox>
#include <QFileInfo>
#include <QBitArray>
#include <QClipboard>
#include <QMenu>
#include <QDesktopServices>
#include <QUrl>

CnvWidget::CnvWidget(QString ps_filename, FilterDockWidget* filter_widget, const GeneSet& het_hit_genes, QWidget *parent)
	: QWidget(parent)
	, ui(new Ui::CnvWidget)
	, cnvs()
	, var_het_hit_genes(het_hit_genes)
{
	ui->setupUi(this);
	GUIHelper::styleSplitter(ui->splitter);
	connect(ui->cnvs, SIGNAL(itemDoubleClicked(QTableWidgetItem*)), this, SLOT(cnvDoubleClicked(QTableWidgetItem*)));
	connect(ui->cnvs, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(showContextMenu(QPoint)));
	connect(ui->copy_clipboard, SIGNAL(clicked(bool)), this, SLOT(copyToClipboard()));
	connect(ui->filter_widget, SIGNAL(filtersChanged()), this, SLOT(applyFilters()));

	//set small variant filters
	ui->filter_widget->setVariantFilterWidget(filter_widget);

	//load CNV data file
	QString path = QFileInfo(ps_filename).absolutePath();
	QStringList cnv_files = Helper::findFiles(path, "*_cnvs.tsv", false);
	if (cnv_files.count()==0)
	{
		addInfoLine("<font color='red'>No CNV data file found in directory " + path + "</font>");
		disableGUI();
	}
	else if (cnv_files.count()>1)
	{
		addInfoLine("<font color='red'>Two or more CNV data files found in directory " + path + "</font>");
		disableGUI();
	}
	else
	{
		try
		{
			loadCNVs(cnv_files[0]);
		}
		catch(Exception e)
		{
			addInfoLine("<font color='red'>Error parsing file:\n" + e.message() + "</font>");
			disableGUI();
		}
	}

	//apply filters
	applyFilters();
}

CnvWidget::~CnvWidget()
{
	delete ui;
}

void CnvWidget::cnvDoubleClicked(QTableWidgetItem* item)
{
	if (item==nullptr) return;

	emit openRegionInIGV(cnvs[item->row()].toString());
}

void CnvWidget::addInfoLine(QString text)
{
	ui->info_box->layout()->addWidget(new QLabel(text));
}

void CnvWidget::disableGUI()
{
	ui->cnvs->setEnabled(false);
	ui->filter_widget->setEnabled(false);
}

void CnvWidget::loadCNVs(QString filename)
{
	//load variants from file
	cnvs.load(filename);

	//show comments
	foreach(QByteArray comment, cnvs.comments())
	{
		addInfoLine(comment);
	}

	//add generic annotations
	QVector<int> annotation_indices;
	for(int i=0; i<cnvs.annotationHeaders().count(); ++i)
	{
		QByteArray header = cnvs.annotationHeaders()[i];
		if (header=="size" || header=="region_count") continue;

		ui->cnvs->setColumnCount(ui->cnvs->columnCount() + 1);
		ui->cnvs->setHorizontalHeaderItem(ui->cnvs->columnCount() -1, new QTableWidgetItem(QString(header)));
		annotation_indices.append(i);
	}

	//show variants
	ui->cnvs->setRowCount(cnvs.count());
	for (int r=0; r<cnvs.count(); ++r)
	{
		ui->cnvs->setItem(r, 0, createItem(cnvs[r].toString()));
		ui->cnvs->setItem(r, 1, createItem(QString::number(cnvs[r].size()/1000.0, 'f', 3), Qt::AlignRight|Qt::AlignTop));
		ui->cnvs->setItem(r, 2, createItem(QString::number(cnvs[r].regions()), Qt::AlignRight|Qt::AlignTop));
		ui->cnvs->setItem(r, 3, createItem(QString(cnvs[r].genes().join(','))));

		int c = 4;
		foreach(int index, annotation_indices)
		{
			QTableWidgetItem* item = createItem(cnvs[r].annotations()[index]);
			//special handling for OMIM
			if (cnvs.annotationHeaders()[index]=="omim")
			{
				item->setText(item->text().replace("_", " "));
			}
			item->setToolTip(item->text());
			ui->cnvs->setItem(r, c++, item);
		}
	}

	//resize columns
	GUIHelper::resizeTableCells(ui->cnvs, 200);
}

void CnvWidget::applyFilters()
{
	//init
	QBitArray pass;
	const int rows = cnvs.count();
	pass.fill(true, rows);

	//filter by regions
	const int f_regs = ui->filter_widget->minRegs();
	if (f_regs>1)
	{
		for(int r=0; r<rows; ++r)
		{
			if (!pass[r]) continue;
			pass[r] = cnvs[r].regions() >= f_regs;
		}
	}

	//filter by size
	const double f_size = 1000.0 * ui->filter_widget->minSizeKb();
	if (f_size>0.0)
	{
		for(int r=0; r<rows; ++r)
		{
			if (!pass[r]) continue;
			pass[r] = cnvs[r].size() >= f_size;
		}
	}

/*TODO
	//filter by copy-number
	QString f_cn = ui->f_cn->currentText();
	if (f_cn!="n/a")
	{
		int f_cni = f_cn.left(1).toInt();
		for(int r=0; r<rows; ++r)
		{
			if (!pass[r]) continue;
			bool hit = false;
			foreach (int cn, cnvs[r].copyNumbers())
			{
				if (cn==f_cni)
				{
					hit = true;
					break;
				}
				if (f_cni==4 && cn>4)
				{
					hit = true;
					break;
				}
			}
			pass[r] = hit;
		}
	}

	//filter by z-score
	const double f_z = ui->f_z->value();
	if (f_z>0.0)
	{
		for(int r=0; r<rows; ++r)
		{
			if (!pass[r]) continue;
			bool hit = false;
			foreach (double z, cnvs[r].zScores())
			{
				if (z>=f_z || z<=-f_z)
				{
					hit = true;
					break;
				}
			}
			pass[r] = hit;
		}
	}

	//filter by allele frequency
	const double f_af = ui->f_af->value();
	for(int r=0; r<rows; ++r)
	{
		if (!pass[r]) continue;
		bool hit = false;
		foreach (double af, cnvs[r].alleleFrequencies())
		{
			if (af<f_af)
			{
				hit = true;
				break;
			}
		}
		pass[r] = hit;
	}

	//filter comp-het
	if (ui->f_comphet->currentText()!="n/a")
	{
		//count hits per gene for CNVs
		QMap<QByteArray, int> gene_count;
		for(int r=0; r<rows; ++r)
		{
			foreach(const QByteArray& gene, cnvs[r].genes())
			{
				if (!pass[r]) continue;
				gene_count[gene] += 1;
			}
		}

		//two CNV hits
		GeneSet comphet_hit;
		if (ui->f_comphet->currentText()=="CNV-CNV")
		{
			for(auto it=gene_count.cbegin(); it!=gene_count.cend(); ++it)
			{
				if (it.value()>1)
				{
					comphet_hit.insert(it.key());
				}
			}
		}

		//one CNV and one SNV/INDEL hit
		else
		{
			GeneSet single_hit_cnv;
			for(auto it=gene_count.cbegin(); it!=gene_count.cend(); ++it)
			{
				if (it.value()==1)
				{
					single_hit_cnv.insert(it.key());
				}
			}

			if (Settings::boolean("NGSD_enabled", true))
			{
				NGSD db;
				single_hit_cnv = db.genesToApproved(single_hit_cnv, true);
			}

			foreach(const QByteArray& gene, single_hit_cnv)
			{
				if (var_het_hit_genes.contains(gene))
				{
					comphet_hit.insert(gene);
				}
			}
		}

		//flag passing CNVs
		for(int r=0; r<rows; ++r)
		{
			if (!pass[r]) continue;

			pass[r] = cnvs[r].genes().intersectsWith(comphet_hit);
		}
	}
*/

	//filter by genes
	GeneSet genes = ui->filter_widget->genes();
	if (!genes.isEmpty())
	{
		QByteArray genes_joined = genes.join('|');

		if (genes_joined.contains("*")) //with wildcards
		{
			QRegExp reg(genes_joined.replace("-", "\\-").replace("*", "[A-Z0-9-]*"));
			for(int r=0; r<rows; ++r)
			{
				if (!pass[r]) continue;

				bool match_found = false;
				foreach(const QByteArray& cnv_gene, cnvs[r].genes())
				{
					if (reg.exactMatch(cnv_gene))
					{
						match_found = true;
						break;
					}
				}
				pass[r] = match_found;
			}
		}
		else //without wildcards
		{
			for(int r=0; r<rows; ++r)
			{
				if (!pass[r]) continue;

				pass[r] = cnvs[r].genes().intersectsWith(genes);
			}
		}
	}

	//filter by ROI
	QString roi_file = ui->filter_widget->targetRegion();
	if (roi_file!="")
	{
		BedFile roi;
		roi.load(roi_file);
		for(int r=0; r<rows; ++r)
		{
			if (!pass[r]) continue;

			pass[r] = roi.overlapsWith(cnvs[r].chr(), cnvs[r].start(), cnvs[r].end());
		}
	}

	//filter by region
	QString region_text = ui->filter_widget->region();
	BedLine region = BedLine::fromString(region_text);
	if (!region.isValid()) //check if valid chr
	{
		Chromosome chr(region_text);
		if (chr.isNonSpecial())
		{
			region.setChr(chr);
			region.setStart(1);
			region.setEnd(999999999);
		}
	}
	if (region.isValid()) //valid region (chr,start, end or only chr)
	{
		for(int r=0; r<rows; ++r)
		{
			if (!pass[r]) continue;

			pass[r] = region.overlapsWith(cnvs[r].chr(), cnvs[r].start(), cnvs[r].end());
		}
	}

	//filter by phenotype (via genes, not genomic regions)
	QList<Phenotype> phenotypes = ui->filter_widget->phenotypes();
	if (!phenotypes.isEmpty())
	{
		NGSD db;
		GeneSet pheno_genes;
		foreach(const Phenotype& pheno, phenotypes)
		{
			pheno_genes << db.phenotypeToGenes(pheno, true);
		}

		for(int r=0; r<rows; ++r)
		{
			if (!pass[r]) continue;

			pass[r] = cnvs[r].genes().intersectsWith(pheno_genes);
		}
	}

	//filter annotations by text
	QByteArray text = ui->filter_widget->text().trimmed().toLower();
	if (text!="")
	{
		for(int r=0; r<rows; ++r)
		{
			if (!pass[r]) continue;

			bool match = false;
			foreach(const QByteArray& anno, cnvs[r].annotations())
			{
				if (anno.toLower().contains(text))
				{
					match = true;
					break;
				}
			}
			pass[r] = match;
		}
	}

	//update GUI
	for(int r=0; r<rows; ++r)
	{
		ui->cnvs->setRowHidden(r, !pass[r]);
	}
	updateStatus(pass.count(true));
}

void CnvWidget::copyToClipboard()
{
	GUIHelper::copyToClipboard(ui->cnvs);
}

void CnvWidget::showContextMenu(QPoint p)
{
	//make sure a row was clicked
	int row = ui->cnvs->indexAt(p).row();
	if (row==-1) return;

	//create menu
	QMenu menu;
	menu.addAction(QIcon("://Icons/DGV.png"), "Open in DGV");
	menu.addAction(QIcon("://Icons/UCSC.png"), "Open in UCSC Genome Browser");

	QAction* action = menu.addAction("Open OMIM entries");
	int omim_index = cnvs.annotationHeaders().indexOf("omim");
	QString omim_text = cnvs[row].annotations()[omim_index].trimmed();
	action->setEnabled(!omim_text.isEmpty());

	//exec menu
	action = menu.exec(ui->cnvs->viewport()->mapToGlobal(p));
	if (action==nullptr) return;
	QString text = action->text();

	//DGV
	if (text=="Open in DGV")
	{
		QDesktopServices::openUrl(QUrl("http://dgv.tcag.ca/gb2/gbrowse/dgv2_hg19/?name=" + cnvs[row].toString()));
	}

	//UCSC
	if (text=="Open in UCSC Genome Browser")
	{
		QDesktopServices::openUrl(QUrl("http://genome.ucsc.edu/cgi-bin/hgTracks?db=hg19&position=" + cnvs[row].toString()));
	}

	//OMIM
	if (text=="Open OMIM entries")
	{
		auto entries = VariantDetailsDockWidget::parseDB(omim_text, "],");
		foreach(VariantDetailsDockWidget::DBEntry entry, entries)
		{
			QDesktopServices::openUrl(QUrl("http://omim.org/entry/" + entry.id));
		}
	}
}

void CnvWidget::updateStatus(int shown)
{
	QString text = QString::number(shown) + "/" + QString::number(cnvs.count()) + " passing filter(s)";
	ui->status->setText(text);
}

QTableWidgetItem* CnvWidget::createItem(QString text, int alignment)
{
	QTableWidgetItem* item = new QTableWidgetItem(text);
	item->setTextAlignment(alignment);

	return item;
}


