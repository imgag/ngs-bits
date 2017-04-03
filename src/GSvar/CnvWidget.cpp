#include "CnvWidget.h"
#include "ui_CnvWidget.h"
#include "Helper.h"
#include "Exceptions.h"
#include <QFileInfo>
#include <QBitArray>
#include <QClipboard>

CnvWidget::CnvWidget(QString filename, QWidget *parent)
	: QWidget(parent)
	, ui(new Ui::CnvList)
	, cnvs()
	, f_genes()
{
	ui->setupUi(this);
	connect(ui->cnvs, SIGNAL(itemDoubleClicked(QTableWidgetItem*)), this, SLOT(cnvDoubleClicked(QTableWidgetItem*)));
	connect(ui->copy_clipboard, SIGNAL(clicked(bool)), this, SLOT(copyToClipboard()));

	//connect
	connect(ui->f_regs, SIGNAL(valueChanged(int)), this, SLOT(filtersChanged()));
	connect(ui->f_cn, SIGNAL(currentIndexChanged(int)), this, SLOT(filtersChanged()));
	connect(ui->f_roi, SIGNAL(stateChanged(int)), this, SLOT(filtersChanged()));
	connect(ui->f_genes, SIGNAL(stateChanged(int)), this, SLOT(filtersChanged()));
	connect(ui->f_size, SIGNAL(valueChanged(double)), this, SLOT(filtersChanged()));

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
}

void CnvWidget::loadCNVs(QString filename)
{
	//load variants from file
	cnvs.load(filename);

	//show comments
	foreach(QString comment, cnvs.comments())
	{
		addInfoLine(comment);
	}

	//show variants
	ui->cnvs->setRowCount(cnvs.count());
	for (int r=0; r<cnvs.count(); ++r)
	{
		ui->cnvs->setItem(r, 0, new QTableWidgetItem(cnvs[r].toString()));
		ui->cnvs->setItem(r, 1, new QTableWidgetItem(cnvs[r].genes().join(",")));
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
	}

	//resize columns
	ui->cnvs->resizeColumnsToContents();
	ui->cnvs->resizeRowsToContents();
	for(int c=0; c<ui->cnvs->columnCount(); ++c)
	{
		if (ui->cnvs->columnWidth(c)>200)
		{
			ui->cnvs->setColumnWidth(c, 200);
		}
	}

	updateStatus(cnvs.count());
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

	//filter by genes
	if (ui->f_genes->isChecked())
	{
		for(int r=0; r<rows; ++r)
		{
			if (!pass[r]) continue;
			bool hit = false;
			foreach(QString gene, f_genes)
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
		for (int col=0; col<ui->cnvs->columnCount(); ++col)
		{
			if (col!=0) output += "\t";
			output += ui->cnvs->item(row, col)->text();
		}
		output += "\n";
	}

	QApplication::clipboard()->setText(output);
}

void CnvWidget::updateStatus(int shown)
{
	QString text = QString::number(shown) + "/" + QString::number(cnvs.count()) + " remaining";
	ui->status->setText(text);
}

