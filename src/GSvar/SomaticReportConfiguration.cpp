#include "SomaticReportConfiguration.h"
#include "ui_SomaticReportConfiguration.h"
#include "GUIHelper.h"
#include <QMenu>
#include "cmath"

SomaticReportConfiguration::SomaticReportConfiguration(const ClinCnvList& cnv_input, GeneSet keep_genes, QWidget *parent)
	: QDialog(parent)
	, ui_(new Ui::SomaticReportConfiguration)
	, cnvs_(cnv_input)
	, keep_genes_cnv_(keep_genes)
{
	ui_->setupUi(this);

	//set up CNV table
	QStringList col_names;
	col_names << "" <<  "chr" << "start" << "end" << "copy number" <<  "logarithmic likelihood" << "genes";
	ui_->cnvs->setColumnCount(col_names.count());
	ui_->cnvs->setHorizontalHeaderLabels(col_names);

	//load data into QTableWidget cnvs
	ui_->cnvs->setRowCount(cnvs_.count());
	for(int row=0;row<cnvs_.count();++row)
	{
		const ClinCopyNumberVariant& cnv = cnvs_[row];

		//checkbox
		ui_->cnvs->setItem(row, 0, new QTableWidgetItem(""));
		ui_->cnvs->item(row, 0)->setFlags(Qt::ItemIsUserCheckable|Qt::ItemIsEnabled);
		ui_->cnvs->item(row, 0)->setTextAlignment(Qt::AlignCenter);
		ui_->cnvs->item(row, 0)->setCheckState(Qt::Checked);

		//data
		ui_->cnvs->setItem(row, 1, new QTableWidgetItem(QString(cnv.chr().str())));
		ui_->cnvs->setItem(row, 2, new QTableWidgetItem(QString::number(cnv.start())));
		ui_->cnvs->setItem(row, 3, new QTableWidgetItem(QString::number(cnv.end())));
		ui_->cnvs->setItem(row, 4, new QTableWidgetItem(QString::number(cnv.copyNumber(), 'f', 1)));
		ui_->cnvs->item(row, 4)->setTextAlignment(Qt::AlignRight|Qt::AlignCenter);
		ui_->cnvs->setItem(row, 5, new QTableWidgetItem(QString::number(cnv.likelihood(), 'f', 1)));
		ui_->cnvs->item(row, 5)->setTextAlignment(Qt::AlignRight|Qt::AlignCenter);
		ui_->cnvs->setItem(row, 6, new QTableWidgetItem(QString(cnv.genes().join(", "))));
	}
	GUIHelper::resizeTableCells(ui_->cnvs, 600);

	//buttons
	connect(ui_->buttonBox,SIGNAL(accepted()),this,SLOT(accept()));
	connect(ui_->buttonBox,SIGNAL(rejected()),this,SLOT(reject()));

	//context menu
	ui_->cnvs->setContextMenuPolicy(Qt::CustomContextMenu);
	connect(ui_->cnvs, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(showContextMenu(QPoint)));
}

SomaticReportConfiguration::~SomaticReportConfiguration()
{
	delete ui_;
}

ClinCnvList SomaticReportConfiguration::getSelectedCNVs()
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
