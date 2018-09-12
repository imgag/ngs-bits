#include "SomaticReportConfiguration.h"
#include "ui_SomaticReportConfiguration.h"
#include "GUIHelper.h"
#include <QMenu>
#include "cmath"

SomaticReportConfiguration::SomaticReportConfiguration(const ClinCnvList& cnv_input,GeneSet keep_genes,QWidget *parent) :
	QDialog(parent)
	, ui_(new Ui::SomaticReportConfiguration)
	, cnvs_(cnv_input)
	, keep_genes_cnv_(keep_genes)
{
	ui_->setupUi(this);
	QStringList col_names;

	col_names << "" <<  "chr" << "start" << "end" << "Copy Number" <<  "Logarithmic Likelihood" << "genes";
	ui_->cnvs->setColumnCount(col_names.count());
	ui_->cnvs->setColumnWidth(0,27); //checkbox
	ui_->cnvs->setColumnWidth(1,60); //chr
	ui_->cnvs->setColumnWidth(4,80); //copy number
	ui_->cnvs->setColumnWidth(5,200); //logarithmic likelihood

	ui_->cnvs->setHorizontalHeaderLabels(col_names);


	ui_->cnvs->setRowCount(cnvs_.count());

	ui_->cnvs->setAlternatingRowColors(true);

	ui_->cnvs->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOn);


	//load data into QTableWidget cnvs
	for(int row=0;row<cnvs_.count();++row)
	{
		ClinCopyNumberVariant cnv = cnvs_[row];

		ui_->cnvs->setItem(row, 0, new QTableWidgetItem(""));

		ui_->cnvs->item(row, 0)->setFlags(Qt::ItemIsUserCheckable|Qt::ItemIsEnabled);
		ui_->cnvs->item(row,0)->setTextAlignment(Qt::AlignCenter);


		ui_->cnvs->item(row, 0)->setCheckState(Qt::Checked);

		ui_->cnvs->setItem(row, 1, new QTableWidgetItem(QString(cnv.chr().str())));
		ui_->cnvs->setItem(row, 2, new QTableWidgetItem(QString::number(cnv.start())));
		ui_->cnvs->setItem(row, 3, new QTableWidgetItem(QString::number(cnv.end())));
		ui_->cnvs->setItem(row, 4, new QTableWidgetItem(QString::number(cnv.copyNumber())));
		ui_->cnvs->setItem(row, 5, new QTableWidgetItem(QString::number(cnv.likelihood())));

		QString genes = "";
		GeneSet gene_list = cnv.genes();
		for(int i=0;i<gene_list.count();++i)
		{
			genes += gene_list[i];
			if(i<gene_list.count()-1) genes += ", ";
		}
		ui_->cnvs->setItem(row,6,new QTableWidgetItem(genes));
	}

	ui_->cnvs->resizeColumnToContents(7);

	connect(ui_->buttonBox,SIGNAL(accepted()),this,SLOT(accept()) );
	connect(ui_->buttonBox,SIGNAL(rejected()),this,SLOT(reject()) );
	//context menu
	ui_->cnvs->setContextMenuPolicy(Qt::CustomContextMenu);
	connect(ui_->cnvs, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(showContextMenu(QPoint)));
}

SomaticReportConfiguration::~SomaticReportConfiguration()
{
	delete ui_;
}

ClinCnvList SomaticReportConfiguration::getFilteredVariants()
{
	ClinCnvList filtered_cnvs;

	filtered_cnvs.copyMetaData(cnvs_);

	for(int row=0;row<ui_->cnvs->rowCount();++row)
	{
		if(ui_->cnvs->item(row,0)->checkState() == Qt::CheckState::Checked)
		{
			filtered_cnvs.append(cnvs_[row]);
		}
	}
	return filtered_cnvs;
}

void SomaticReportConfiguration::showContextMenu(QPoint pos)
{
	QMenu menu(ui_->cnvs);

	menu.addAction(QIcon(":/Icons/box_checked.png"), "select all");
	menu.addAction(QIcon(":/Icons/box_unchecked.png"), "unselect all");

	QAction* action = menu.exec(ui_->cnvs->viewport()->mapToGlobal(pos));
	if(action==nullptr) return;

	QByteArray text = action->text().toLatin1();
	if (text=="select all")
	{
		for (int i=0; i<ui_->cnvs->rowCount(); ++i)
		{
			ui_->cnvs->item(i, 0)->setCheckState(Qt::Checked);
		}
	}
	if (text=="unselect all")
	{
		for (int i=0; i<ui_->cnvs->rowCount(); ++i)
		{
			ui_->cnvs->item(i, 0)->setCheckState(Qt::Unchecked);
		}
	}
}
