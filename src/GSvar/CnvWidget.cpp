#include "CnvWidget.h"
#include "ui_CnvWidget.h"
#include "Helper.h"
#include "Exceptions.h"
#include "GUIHelper.h"
#include "VariantDetailsDockWidget.h"
#include <QFileInfo>
#include <QBitArray>
#include <QClipboard>
#include <QMenu>
#include <QDesktopServices>
#include <QUrl>

CnvWidget::CnvWidget(QString filename, QWidget *parent)
	: QWidget(parent)
	, ui(new Ui::CnvList)
	, cnvs()
	, f_genes()
{
	ui->setupUi(this);
	connect(ui->cnvs, SIGNAL(itemDoubleClicked(QTableWidgetItem*)), this, SLOT(cnvDoubleClicked(QTableWidgetItem*)));
	connect(ui->cnvs, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(showContextMenu(QPoint)));
	connect(ui->copy_clipboard, SIGNAL(clicked(bool)), this, SLOT(copyToClipboard()));

	//connect
	connect(ui->f_regs, SIGNAL(valueChanged(int)), this, SLOT(filtersChanged()));
	connect(ui->f_cn, SIGNAL(currentIndexChanged(int)), this, SLOT(filtersChanged()));
	connect(ui->f_roi, SIGNAL(stateChanged(int)), this, SLOT(filtersChanged()));
	connect(ui->f_genes, SIGNAL(stateChanged(int)), this, SLOT(filtersChanged()));
	connect(ui->f_size, SIGNAL(valueChanged(double)), this, SLOT(filtersChanged()));
	connect(ui->f_z, SIGNAL(valueChanged(double)), this, SLOT(filtersChanged()));
	connect(ui->f_af, SIGNAL(valueChanged(double)), this, SLOT(filtersChanged()));
	connect(ui->f_anno_name, SIGNAL(currentIndexChanged(int)), this, SLOT(filtersChanged()));
	connect(ui->f_anno_name, SIGNAL(currentIndexChanged(int)), this, SLOT(annotationFilterColumnChanged()));
	connect(ui->f_anno_op, SIGNAL(currentIndexChanged(int)), this, SLOT(filtersChanged()));
	connect(ui->f_anno_op, SIGNAL(currentIndexChanged(int)), this, SLOT(annotationFilterOperationChanged()));
	connect(ui->f_anno_value, SIGNAL(textEdited(QString)), this, SLOT(filtersChanged()));

	//load CNV data file
	QString path = QFileInfo(filename).absolutePath();
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
		loadCNVs(cnv_files[0]);
	}
}

CnvWidget::~CnvWidget()
{
	delete ui;
}

void CnvWidget::setGenesFilter(const GeneSet& genes)
{
	if (genes.isEmpty())
	{
		ui->f_genes->setEnabled(false);
		f_genes.clear();
	}
	else
	{
		ui->f_genes->setEnabled(true);
		f_genes = genes;
	}
}

void CnvWidget::setRoiFilter(QString filename)
{
	if (filename=="")
	{
		ui->f_roi->setEnabled(false);
		f_roi.clear();
	}
	else
	{
		ui->f_roi->setEnabled(true);
		f_roi.load(filename);
	}
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
	ui->f_cn->setEnabled(false);
	ui->f_genes->setEnabled(false);
	ui->f_roi->setEnabled(false);
	ui->f_regs->setEnabled(false);
	ui->f_size->setEnabled(false);
	ui->f_z->setEnabled(false);
	ui->f_af->setEnabled(false);
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

	//add generic annotation filter names
	ui->f_anno_name->clear();
	ui->f_anno_name->addItem("");
	foreach(int index, annotation_indices)
	{
		ui->f_anno_name->addItem(cnvs.annotationHeaders()[index], index);
	}

	//show variants
	ui->cnvs->setRowCount(cnvs.count());
	for (int r=0; r<cnvs.count(); ++r)
	{
		ui->cnvs->setItem(r, 0, new QTableWidgetItem(cnvs[r].toString()));
		ui->cnvs->setItem(r, 1, new QTableWidgetItem(QString(cnvs[r].genes().join(','))));
		ui->cnvs->setItem(r, 2, new QTableWidgetItem(QString::number(cnvs[r].size()/1000.0, 'f', 3)));
		ui->cnvs->setItem(r, 3, new QTableWidgetItem(QString::number(cnvs[r].regionCount())));

		QStringList tmp;
		foreach (int cn, cnvs[r].copyNumbers())
		{
			tmp.append(QString::number(cn));
		}
		ui->cnvs->setItem(r, 4, new QTableWidgetItem(tmp.join(",")));

		tmp.clear();
		foreach (double z, cnvs[r].zScores())
		{
			tmp.append(QString::number(z));
		}
		ui->cnvs->setItem(r, 5, new QTableWidgetItem(tmp.join(",")));

		tmp.clear();
		foreach (double af, cnvs[r].alleleFrequencies())
		{
			tmp.append(QString::number(af, 'f', 3));
		}
		ui->cnvs->setItem(r, 6, new QTableWidgetItem(tmp.join(",")));

		int c = 7;
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
	GUIHelper::resizeTableCells(ui->cnvs, 180);

	//apply default filters (and update status)
	filtersChanged();
}

void CnvWidget::filtersChanged()
{
	//init
	QBitArray pass;
	const int rows = cnvs.count();
	pass.fill(true, rows);

	//filter by regions
	const int f_regs = ui->f_regs->value();
	if (f_regs>1)
	{
		for(int r=0; r<rows; ++r)
		{
			if (!pass[r]) continue;
			pass[r] = cnvs[r].regionCount() >= f_regs;
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

	//filter by genes
	if (ui->f_genes->isChecked())
	{
		for(int r=0; r<rows; ++r)
		{
			if (!pass[r]) continue;
			bool hit = false;
			foreach(const QByteArray& gene, f_genes)
			{
				if (cnvs[r].genes().contains(gene))
				{
					hit = true;
					break;
				}
			}
			pass[r] = hit;
		}
	}

	//filter by ROI
	if (ui->f_roi->isChecked())
	{
		for(int r=0; r<rows; ++r)
		{
			if (!pass[r]) continue;
			pass[r] = f_roi.overlapsWith(cnvs[r].chr(), cnvs[r].start(), cnvs[r].end());
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
			QByteArray anno_value = ui->f_anno_value->text().toLatin1();
			for(int r=0; r<rows; ++r)
			{
				if (!pass[r]) continue;
				pass[r] = cnvs[r].annotations()[anno_col_index].contains(anno_value);
			}
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
	//header
	QString output = "#";
	for (int col=0; col<ui->cnvs->columnCount(); ++col)
	{
		if (col!=0) output += "\t";
		output += ui->cnvs->horizontalHeaderItem(col)->text();
	}
	output += "\n";

	//rows
	for (int row=0; row<ui->cnvs->rowCount(); ++row)
	{
		if (ui->cnvs->isRowHidden(row)) continue;

		for (int col=0; col<ui->cnvs->columnCount(); ++col)
		{
			if (col!=0) output += "\t";
			output += ui->cnvs->item(row, col)->text();
		}
		output += "\n";
	}

	QApplication::clipboard()->setText(output);
}

void CnvWidget::annotationFilterColumnChanged()
{
	ui->f_anno_op->setEnabled(ui->f_anno_name->currentText()!="");
}

void CnvWidget::annotationFilterOperationChanged()
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

void CnvWidget::showContextMenu(QPoint p)
{
	//make sure a row was clicked
	int row = ui->cnvs->indexAt(p).row();
	if (row==-1) return;


	//create menu
	QMenu menu;
	menu.addAction("Open in DGV");
	menu.addAction("Open in UCSC Genome Browser");

	int omim_index = cnvs.annotationHeaders().indexOf("omim");
	if (omim_index!=-1)
	{
		auto entries = VariantDetailsDockWidget::parseDB(cnvs[row].annotations()[omim_index]);
		foreach(VariantDetailsDockWidget::DBEntry entry, entries)
		{
			menu.addAction("Open OMIM: " + entry.id);
		}
	}

	//exec menu
	QAction* action = menu.exec(ui->cnvs->viewport()->mapToGlobal(p));
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
	if (text.startsWith("Open OMIM:"))
	{
		QString id = text.mid(text.indexOf(":")+2);
		QDesktopServices::openUrl(QUrl("http://omim.org/entry/" + id));
	}

}

void CnvWidget::updateStatus(int shown)
{
	QString text = QString::number(shown) + "/" + QString::number(cnvs.count()) + " passing filter(s)";
	ui->status->setText(text);
}

