#include "ClinCnvWidget.h"
#include "ui_ClinCnvWidget.h"
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

ClinCnvWidget::ClinCnvWidget(QString filename, FilterDockWidget* filter_widget, const GeneSet& het_hit_genes, QWidget *parent)
  : QWidget(parent)
  , var_filters_(filter_widget)
  , var_het_hit_genes_(het_hit_genes)
  , ui(new Ui::ClinCnvWidget)
  , cnvs()
{

	ui->setupUi(this);

	connect(ui->cnvs, SIGNAL(itemDoubleClicked(QTableWidgetItem*)), this, SLOT(cnvDoubleClicked(QTableWidgetItem*)));
	connect(ui->cnvs, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(showContextMenu(QPoint)));
	connect(ui->copy_clipboard,SIGNAL(clicked(bool)), this, SLOT(copyToClipboard()));

	//connect filters
	connect(ui->f_regs, SIGNAL(valueChanged(int)), this, SLOT(filtersChanged()));
	connect(ui->f_cn, SIGNAL(activated(int)), this, SLOT(filtersChanged()));
	connect(ui->f_roi, SIGNAL(stateChanged(int)), this, SLOT(filtersChanged()));
	connect(ui->f_genes, SIGNAL(stateChanged(int)), this, SLOT(filtersChanged()));
	connect(ui->f_pheno, SIGNAL(stateChanged(int)), this, SLOT(filtersChanged()));
	connect(ui->f_text, SIGNAL(stateChanged(int)), this, SLOT(filtersChanged()));
	connect(ui->f_comphet, SIGNAL(currentIndexChanged(int)), this, SLOT(filtersChanged()));
	connect(ui->f_size, SIGNAL(valueChanged(double)), this, SLOT(filtersChanged()));
	connect(ui->f_likeli, SIGNAL(valueChanged(double)), this, SLOT(filtersChanged()));
	connect(ui->f_af, SIGNAL(valueChanged(double)), this, SLOT(filtersChanged()));
	connect(ui->f_anno_name, SIGNAL(currentIndexChanged(int)), this, SLOT(filtersChanged()));
	connect(ui->f_anno_name, SIGNAL(currentIndexChanged(int)), this, SLOT(annotationFilterColumnChanged()));
	connect(ui->f_anno_op, SIGNAL(currentIndexChanged(int)), this, SLOT(filtersChanged()));
	connect(ui->f_anno_op, SIGNAL(currentIndexChanged(int)), this, SLOT(annotationFilterOperationChanged()));
	connect(ui->f_anno_value, SIGNAL(textEdited(QString)), this, SLOT(filtersChanged()));
	connect(var_filters_, SIGNAL(filtersChanged()), this, SLOT(variantFiltersChanged()));
	connect(var_filters_, SIGNAL(targetRegionChanged()), this, SLOT(variantFiltersChanged()));


	//load ClinCNV data file
	QString path = QFileInfo(filename).absolutePath();
	QStringList cnv_files = Helper::findFiles(path, "*_clincnv.tsv", false);
	if (cnv_files.count()==0)
	{
		addInfoLine("<font color='red'>No ClinCNV data file found in directory " + path + "</font>");
		disableGUI();
	}
	else if (cnv_files.count()>1)
	{
		addInfoLine("<font color='red'>Two or more ClinCNV data files found in directory " + path + "</font>");
		disableGUI();
	}
	else
	{
		loadCNVs(cnv_files[0]);
	}
	is_somatic_ = (cnvs.annotationIndexByName("tumor_CN_change",false) != -1);

	//Disable certain filters if it is a somatic sample
	if(is_somatic_)
	{
		//ui->f_cn->setEnabled(false);
		ui->f_cn->clear();
		ui->f_cn->addItems({"n/a","0-1","1-2","2-3","3+"});
		ui->f_cn->setCurrentText("n/a");

		ui->f_af->setEnabled(false);
		ui->f_regs->setEnabled(false);
		ui->f_comphet->setEnabled(false);
	}

	//update variant list dependent filters (and apply filters)
	variantFiltersChanged();
}

void ClinCnvWidget::loadCNVs(QString filename)
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
		if(cnvs.annotationHeaders()[i] == "length_KB") continue;

		QByteArray header = cnvs.annotationHeaders()[i];
		ui->cnvs->setColumnCount(ui->cnvs->columnCount() + 1);
		ui->cnvs->setHorizontalHeaderItem(ui->cnvs->columnCount() -1, new QTableWidgetItem(QString(header)));
		annotation_indices.append(i);
	}

	//add generic annotation filter names
	ui->f_anno_name->clear();
	ui->f_anno_name->addItem("");
	foreach(int index, annotation_indices)
	{
		//Skip annotation already included in header
		if(cnvs.annotationHeaders()[index] == "potential_AF") continue;
		if(cnvs.annotationHeaders()[index] == "no_of_regions") continue;
		ui->f_anno_name->addItem(cnvs.annotationHeaders()[index], index);
	}


	//show variants
	ui->cnvs->setRowCount(cnvs.count());
	for (int r=0; r<cnvs.count(); ++r)
	{
		ui->cnvs->setItem(r, 0, new QTableWidgetItem(cnvs[r].toString()));
		ui->cnvs->setItem(r, 1, new QTableWidgetItem(QString(cnvs[r].genes().join(','))));
		ui->cnvs->setItem(r, 2, new QTableWidgetItem(QString::number(cnvs[r].size()/1000.0, 'f', 3)));
		ui->cnvs->setItem(r, 3, new QTableWidgetItem(QString::number(cnvs[r].copyNumber())));

		ui->cnvs->setItem(r, 4, new QTableWidgetItem(QString::number(cnvs[r].likelihood())));

		int c = 5;
		foreach(int index, annotation_indices)
		{
			QTableWidgetItem* item = new QTableWidgetItem(QString(cnvs[r].annotations()[index]));
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

void ClinCnvWidget::addInfoLine(QString text)
{
	ui->info_box->layout()->addWidget(new QLabel(text));
}
void ClinCnvWidget::disableGUI()
{
	ui->cnvs->setEnabled(false);
	ui->f_cn->setEnabled(false);
	ui->f_genes->setEnabled(false);
	ui->f_roi->setEnabled(false);
	ui->f_pheno->setEnabled(false);
	ui->f_text->setEnabled(false);
	ui->f_regs->setEnabled(false);
	ui->f_size->setEnabled(false);
	ui->f_likeli->setEnabled(false);
	ui->f_af->setEnabled(false);
}
void ClinCnvWidget::variantFiltersChanged()
{
	ui->f_genes->setEnabled(!var_filters_->genes().isEmpty());
	if (!ui->f_genes->isEnabled()) ui->f_genes->setChecked(false);

	ui->f_roi->setEnabled(!var_filters_->targetRegion().isEmpty());
	if (!ui->f_roi->isEnabled()) ui->f_roi->setChecked(false);

	ui->f_pheno->setEnabled(!var_filters_->phenotypes().isEmpty() && Settings::boolean("NGSD_enabled", true));
	if (!ui->f_pheno->isEnabled()) ui->f_roi->setChecked(false);

	ui->f_text->setEnabled(!var_filters_->text().isEmpty());
	if (!ui->f_text->isEnabled()) ui->f_text->setChecked(false);

	//re-apply filters in case the genes/target region changed
	filtersChanged();
}
void ClinCnvWidget::cnvDoubleClicked(QTableWidgetItem* item)
{
	if (item==nullptr) return;

	emit openRegionInIGV(cnvs[item->row()].toString());
}
void ClinCnvWidget::filtersChanged()
{
	//init
	QBitArray pass;
	const int rows = cnvs.count();
	pass.fill(true, rows);

	//filter by regions
	const int f_regs = ui->f_regs->value();
	int i_no_of_regs = cnvs.annotationIndexByName("no_of_regions",false);
	if (i_no_of_regs != -1 && f_regs>1)
	{
		for(int r=0; r<rows; ++r)
		{
			if (!pass[r]) continue;
			pass[r] = cnvs[r].annotations().at(i_no_of_regs).toInt() >= f_regs;
		}
	}

	//filter by size
	const double f_size = ui->f_size->value();
	if (f_size>0.0)
	{
		for(int r=0; r<rows; ++r)
		{
			if (!pass[r]) continue;
			pass[r] = cnvs[r].size() >= 1000.0 * f_size;
		}
	}

	//filter by copy-number
	QString f_cn = ui->f_cn->currentText();
	if (!is_somatic_ && f_cn!="n/a")
	{
		int f_cni = f_cn.left(1).toInt();
		for(int r=0; r<rows; ++r)
		{
			if (!pass[r]) continue;
			bool hit = false;

			if(f_cni == cnvs[r].copyNumber())
			{
				hit = true;
			}
			if(f_cni==4 && cnvs[r].copyNumber() > 4)
			{
				hit = true;
			}
			pass[r] = hit;
		}
	}

	QString f_som_cn = ui->f_cn->currentText();
	if (is_somatic_ && f_som_cn != "n/a")
	{
		double min;
		double max;
		if(f_som_cn != "3+")
		{
			QList<QString> interval = f_som_cn.split("-");
			min = interval.at(0).toDouble();
			max = interval.at(1).toDouble();
		}
		else
		{
			min = 3;
			max = 1000000;
		}
		for(int r=0;r<rows; ++r)
		{
			if(!pass[r]) continue;
			if(cnvs[r].copyNumber() < min  || cnvs[r].copyNumber() >= max) pass[r] = false;
		}
	}

	//filter by loglikelihood
	const double f_likeli = ui->f_likeli->value();

	if(f_likeli > 0.0)
	{
		for(int r=0;r<rows;++r)
		{
			if(!pass[r]) continue;

			if(cnvs[r].likelihood() >= f_likeli) pass[r] = true;
			else pass[r] = false;
		}
	}

	//filter by allele frequency
	const double f_af = ui->f_af->value();
	const int i_af = cnvs.annotationIndexByName("potential_AF",false);
	if(i_af != -1)
	{
		for(int r=0; r<rows; ++r)
		{
			if (!pass[r]) continue;
			if(!cnvs[r].annotations()[i_af].isEmpty()) pass[r] = cnvs[r].annotations()[i_af].toDouble() <= f_af;
		}
	}


	//filter by genes
	if (ui->f_genes->isChecked())
	{
		GeneSet genes = var_filters_->genes();
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
	if (ui->f_roi->isChecked())
	{
		BedFile roi;
		roi.load(var_filters_->targetRegion());
		for(int r=0; r<rows; ++r)
		{
			if (!pass[r]) continue;

			pass[r] = roi.overlapsWith(cnvs[r].chr(), cnvs[r].start(), cnvs[r].end());
		}
	}

	//filter by phenotype (via genes, not genomic regions)
	if (ui->f_pheno->isChecked())
	{
		NGSD db;
		GeneSet pheno_genes;
		foreach(const Phenotype& pheno, var_filters_->phenotypes())
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
	if (ui->f_text->isChecked())
	{
		QByteArray text = var_filters_->text().trimmed().toLower();

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


	//filter by generic annotation
	if (ui->f_anno_name->currentText()!="")
	{
		int anno_col_index = ui->f_anno_name->currentData().toInt();
		QString anno_op = ui->f_anno_op->currentText();
		if (anno_op=="is empty")
		{
			for(int r=0; r<rows; ++r)
			{
				if (!pass[r]) continue;
				pass[r] = cnvs[r].annotations()[anno_col_index].isEmpty();
			}
		}
		else if (anno_op=="is not empty")
		{
			for(int r=0; r<rows; ++r)
			{
				if (!pass[r]) continue;
				pass[r] = !cnvs[r].annotations()[anno_col_index].isEmpty();
			}
		}
		else
		{
			QByteArray anno_value = ui->f_anno_value->text().toLatin1().toUpper();
			for(int r=0; r<rows; ++r)
			{
				if (!pass[r]) continue;
				pass[r] = cnvs[r].annotations()[anno_col_index].toUpper().contains(anno_value);
			}
		}
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
				single_hit_cnv = db.genesToApproved(single_hit_cnv);
			}

			foreach(const QByteArray& gene, single_hit_cnv)
			{
				if (var_het_hit_genes_.contains(gene))
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

	//update GUI
	for(int r=0; r<rows; ++r)
	{
		ui->cnvs->setRowHidden(r, !pass[r]);
	}
	updateStatus(pass.count(true));
}
void ClinCnvWidget::copyToClipboard()
{
	GUIHelper::copyToClipboard(ui->cnvs);
}
void ClinCnvWidget::annotationFilterColumnChanged()
{
	ui->f_anno_op->setEnabled(ui->f_anno_name->currentText()!="");
}
void ClinCnvWidget::annotationFilterOperationChanged()
{
	if (ui->f_anno_op->currentText()=="contains")
	{
		ui->f_anno_value->setEnabled(true);
	}
	else
	{
		ui->f_anno_value->setEnabled(false);
		ui->f_anno_value->clear();
	}
}
void ClinCnvWidget::showContextMenu(QPoint p)
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
	QString omim_text = "";
	if(omim_index != -1)
	{
		omim_text = cnvs[row].annotations()[omim_index].trimmed();
	}
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

void ClinCnvWidget::updateStatus(int shown)
{
	QString text = QString::number(shown) + "/" + QString::number(cnvs.count()) + " passing filter(s)";
	ui->status->setText(text);
}

ClinCnvWidget::~ClinCnvWidget()
{
	delete ui;
}
