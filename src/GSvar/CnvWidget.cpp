#include "CnvWidget.h"
#include "ui_CnvWidget.h"
#include "Helper.h"
#include "Exceptions.h"
#include "GUIHelper.h"
#include "VariantDetailsDockWidget.h"
#include "NGSD.h"
#include "Settings.h"
#include "Log.h"
#include <QMessageBox>
#include <QFileInfo>
#include <QBitArray>
#include <QClipboard>
#include <QMenu>
#include <QDesktopServices>
#include <QUrl>
#include <QDir>

CnvWidget::CnvWidget(QString gsvar_file, FilterDockWidget* filter_widget, const GeneSet& het_hit_genes, QWidget *parent)
	: QWidget(parent)
	, ui(new Ui::CnvWidget)
	, cnvs()
	, special_cols_()
	, var_het_hit_genes(het_hit_genes)
{
	ui->setupUi(this);
	connect(ui->cnvs, SIGNAL(itemDoubleClicked(QTableWidgetItem*)), this, SLOT(cnvDoubleClicked(QTableWidgetItem*)));
	connect(ui->cnvs, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(showContextMenu(QPoint)));
	connect(ui->copy_clipboard, SIGNAL(clicked(bool)), this, SLOT(copyToClipboard()));
	connect(ui->filter_widget, SIGNAL(filtersChanged()), this, SLOT(applyFilters()));

	//set small variant filters
	ui->filter_widget->setVariantFilterWidget(filter_widget);

	//load CNV data file
	QFileInfo file_info(gsvar_file);
	QString base = file_info.absolutePath() + QDir::separator() + file_info.baseName();
	QString cnv_file = base + "_cnvs_clincnv.tsv";
	if (!QFile::exists(cnv_file)) //fallback to CnvHunter
	{
		cnv_file = base + "_cnvs.tsv";
	}
	try
	{
		loadCNVs(cnv_file);
	}
	catch(Exception e)
	{
		addInfoLine("<font color='red'>Error parsing file:\n" + e.message() + "</font>");
		disableGUI();
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

	int col = item->column();
	int row = item->row();
	QString col_name = item->tableWidget()->horizontalHeaderItem(col)->text();
	if (special_cols_.contains(col_name))
	{
		QString text = cnvs[row].annotations()[col-4].trimmed();
		if (text.isEmpty()) return;
		QString title = col_name + " of CNV " + cnvs[row].toString();

		if (col_name=="cn_pathogenic")
		{
			showSpecialTable(title, text, "");
		}
		if (col_name=="dosage_sensitive_disease_genes")
		{
			showSpecialTable(title, text, "https://www.ncbi.nlm.nih.gov/projects/dbvar/clingen/clingen_gene.cgi?sym=");
		}
		if (col_name=="clinvar_cnvs")
		{
			showSpecialTable(title, text, "https://www.ncbi.nlm.nih.gov/clinvar/variation/");
		}
		if (col_name=="hgmd_cnvs")
		{
			showSpecialTable(title, text, "https://portal.biobase-international.com/hgmd/pro/mut.php?acc=");
		}
		if (col_name=="omim")
		{
			text = text.replace('_', ' ');
			showSpecialTable(title, text, "https://www.omim.org/entry/");
		}
	}
	else
	{
		emit openRegionInIGV(cnvs[item->row()].toString());
	}
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

	//set special columns
	special_cols_ = QStringList() << "cn_pathogenic" << "dosage_sensitive_disease_genes" << "clinvar_cnvs" << "hgmd_cnvs" << "omim";

	//show comments
	foreach(const QByteArray& comment, cnvs.comments())
	{
		addInfoLine(comment);
	}

	//add generic annotations
	QVector<int> annotation_indices;
	for(int i=0; i<cnvs.annotationHeaders().count(); ++i)
	{
		QString header = cnvs.annotationHeaders()[i];
		if (header=="size" || header=="region_count") continue; //CnvHunter germline special handling

		ui->cnvs->setColumnCount(ui->cnvs->columnCount() + 1);
		QTableWidgetItem* item = new QTableWidgetItem(header);
		if (special_cols_.contains(header))
		{
			item->setIcon(QIcon("://Icons/Table.png"));
			item->setToolTip("Double click table cell to open summary table");
		}
		ui->cnvs->setHorizontalHeaderItem(ui->cnvs->columnCount() -1, item);
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
			item->setToolTip(item->text().replace("],","]\n"));
			ui->cnvs->setItem(r, c++, item);
		}
	}

	//resize columns
	GUIHelper::resizeTableCells(ui->cnvs, 200);
}

/*TODO

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

void CnvWidget::applyFilters(bool debug_time)
{
	const int rows = cnvs.count();
	FilterResult filter_result(rows);

	try
	{
		//apply main filter
		QTime timer;
		timer.start();

		const FilterCascade& filter_cascade = ui->filter_widget->filters();
		filter_result = filter_cascade.apply(cnvs, false, debug_time);
		ui->filter_widget->markFailedFilters();

		if (debug_time)
		{
			Log::perf("Applying annotation filters took ", timer);
			timer.start();
		}

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
					if (!filter_result.flags()[r]) continue;

					bool match_found = false;
					foreach(const QByteArray& cnv_gene, cnvs[r].genes())
					{
						if (reg.exactMatch(cnv_gene))
						{
							match_found = true;
							break;
						}
					}
					filter_result.flags()[r] = match_found;
				}
			}
			else //without wildcards
			{
				for(int r=0; r<rows; ++r)
				{
					if (!filter_result.flags()[r]) continue;

					filter_result.flags()[r] = cnvs[r].genes().intersectsWith(genes);
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
				if (!filter_result.flags()[r]) continue;

				filter_result.flags()[r] = roi.overlapsWith(cnvs[r].chr(), cnvs[r].start(), cnvs[r].end());
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
				if (!filter_result.flags()[r]) continue;

				filter_result.flags()[r] = region.overlapsWith(cnvs[r].chr(), cnvs[r].start(), cnvs[r].end());
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
				if (!filter_result.flags()[r]) continue;

				filter_result.flags()[r] = cnvs[r].genes().intersectsWith(pheno_genes);
			}
		}

		//filter annotations by text
		QByteArray text = ui->filter_widget->text().trimmed().toLower();
		if (text!="")
		{
			for(int r=0; r<rows; ++r)
			{
				if (!filter_result.flags()[r]) continue;

				bool match = false;
				foreach(const QByteArray& anno, cnvs[r].annotations())
				{
					if (anno.toLower().contains(text))
					{
						match = true;
						break;
					}
				}
				filter_result.flags()[r] = match;
			}
		}

	}
	catch(Exception& e)
	{
		QMessageBox::warning(this, "Filtering error", e.message() + "\nA possible reason for this error is an outdated variant list.\nTry re-annotating the NGSD columns.\n If re-annotation does not help, please re-analyze the sample (starting from annotation) in the sample information dialog!");

		filter_result = FilterResult(cnvs.count(), false);
	}

	//update GUI
	for(int r=0; r<rows; ++r)
	{
		ui->cnvs->setRowHidden(r, !filter_result.flags()[r]);
	}
	updateStatus(filter_result.countPassing());
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
	menu.addAction(QIcon("://Icons/Decipher.png"), "Open in Decipher browser");
	menu.addAction(QIcon("://Icons/DGV.png"), "Open in DGV");
	menu.addAction(QIcon("://Icons/UCSC.png"), "Open in UCSC browser");

	//exec menu
	QAction* action = menu.exec(ui->cnvs->viewport()->mapToGlobal(p));
	if (action==nullptr) return;
	QString text = action->text();


	if (text=="Open in DGV")
	{
		QDesktopServices::openUrl(QUrl("http://dgv.tcag.ca/gb2/gbrowse/dgv2_hg19/?name=" + cnvs[row].toString()));
	}
	else if (text=="Open in UCSC browser")
	{
		QDesktopServices::openUrl(QUrl("http://genome.ucsc.edu/cgi-bin/hgTracks?db=hg19&position=" + cnvs[row].toString()));
	}
	else if (text=="Open in Decipher browser")
	{
		QDesktopServices::openUrl(QUrl("https://decipher.sanger.ac.uk/browser#q/" + cnvs[row].toString()));
	}
}

void CnvWidget::openLink(int row, int col)
{
	auto table = qobject_cast<QTableWidget*>(sender());
	if (table==nullptr) return;
	auto item = table->item(row, col);
	if (item==nullptr) return;

	QString url = item->data(Qt::UserRole).toString();
	if (url.isEmpty()) return;

	QDesktopServices::openUrl(QUrl(url));
}

void CnvWidget::showSpecialTable(QString col, QString text, QByteArray url_prefix)
{
	//determine headers
	auto entries = VariantDetailsDockWidget::parseDB(text, ',');
	QStringList headers;
	headers << "ID";
	foreach(const VariantDetailsDockWidget::DBEntry& entry, entries)
	{
		QList<KeyValuePair> parts = entry.splitByName();
		foreach(const KeyValuePair& pair, parts)
		{
			if (!headers.contains(pair.key))
			{
				headers << pair.key;
			}
		}
	}

	//set up table widget
	QTableWidget* table = new QTableWidget();
	table->setRowCount(entries.count());
	table->setColumnCount(headers.count());
	for(int col=0; col<headers.count(); ++col)
	{
		auto item = new QTableWidgetItem(headers[col]);
		if (col==0 && !url_prefix.isEmpty())
		{
			item->setIcon(QIcon(":/Icons/Link.png"));
			item->setToolTip("Double click cell to open external link for the entry");
		}
		table->setHorizontalHeaderItem(col, item);
	}
	if (!url_prefix.isEmpty())
	{
		connect(table, SIGNAL(cellDoubleClicked(int,int)), this, SLOT(openLink(int,int)));
	}
	table->setEditTriggers(QAbstractItemView::NoEditTriggers);
	table->setAlternatingRowColors(true);
	table->setWordWrap(false);
	table->setSelectionMode(QAbstractItemView::SingleSelection);
	table->setSelectionBehavior(QAbstractItemView::SelectItems);
	table->verticalHeader()->setVisible(false);
	int row = 0;
	foreach(VariantDetailsDockWidget::DBEntry entry, entries)
	{
		QString id = entry.id.trimmed();
		auto item = new QTableWidgetItem(id);
		if (!url_prefix.isEmpty())
		{
			item->setData(Qt::UserRole, url_prefix + id);
		}
		table->setItem(row, 0, item);

		QList<KeyValuePair> parts = entry.splitByName();
		foreach(const KeyValuePair& pair, parts)
		{
			int col = headers.indexOf(pair.key);
			table->setItem(row, col, new QTableWidgetItem(pair.value));
		}

		++row;
	}

	//show table
	auto dlg = GUIHelper::createDialog(table, col);
	dlg->setMinimumSize(1200, 800);
	GUIHelper::resizeTableCells(table);
	dlg->exec();

	//delete
	delete table;
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


