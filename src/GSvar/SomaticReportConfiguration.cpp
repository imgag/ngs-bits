#include "SomaticReportConfiguration.h"
#include "ui_SomaticReportConfiguration.h"
#include "GUIHelper.h"
#include <QDebug>
#include <QMenu>

SomaticReportConfiguration::SomaticReportConfiguration(const CnvList& cnv_input,GeneSet keep_genes,QWidget *parent) :
	QDialog(parent)
	, ui_(new Ui::SomaticReportConfiguration)
	, cnvs_(cnv_input)
	, keep_genes_cnv_(keep_genes)
{
	ui_->setupUi(this);
	QStringList col_names;

	col_names << "" <<  "chr" << "start" << "end" << "region count" <<  "region copy numbers" << "region zscores" << "genes";
	ui_->cnvs->setColumnCount(col_names.count());
	ui_->cnvs->setColumnWidth(0,27); //checkbox
	ui_->cnvs->setColumnWidth(1,60); //chr
	ui_->cnvs->setColumnWidth(4,80); //region count
	ui_->cnvs->setColumnWidth(5,200); //regioncopy numbers
	ui_->cnvs->setColumnWidth(6,200); //zscores

	ui_->cnvs->setHorizontalHeaderLabels(col_names);


	ui_->cnvs->setRowCount(cnvs_.count());

	ui_->cnvs->setAlternatingRowColors(true);

	ui_->cnvs->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOn);


	//load data into QTableWidget cnvs
	for(int row=0;row<cnvs_.count();++row)
	{
		CopyNumberVariant cnv = cnvs_[row];

		ui_->cnvs->setItem(row, 0, new QTableWidgetItem(""));

		ui_->cnvs->item(row, 0)->setFlags(Qt::ItemIsUserCheckable|Qt::ItemIsEnabled);
		ui_->cnvs->item(row,0)->setTextAlignment(Qt::AlignCenter);
		if(preselectVariant(cnv))
		{
			ui_->cnvs->item(row, 0)->setCheckState(Qt::Checked);
		}
		else
		{
			ui_->cnvs->item(row, 0)->setCheckState(Qt::Unchecked);
		}

		ui_->cnvs->setItem(row, 1, new QTableWidgetItem(QString(cnv.chr().str())));
		ui_->cnvs->setItem(row, 2, new QTableWidgetItem(QString::number(cnv.start())));
		ui_->cnvs->setItem(row, 3, new QTableWidgetItem(QString::number(cnv.end())));
		ui_->cnvs->setItem(row, 4, new QTableWidgetItem(QString::number(cnv.regionCount())));

		//region copy numbers
		QList<int> cn_list = cnv.copyNumbers();
		QString cn = "";
		for(int i=0;i<cn_list.count();++i)
		{
			cn += QString::number(cn_list[i]);
			if(i<cn_list.count()-1) cn += ", ";
		}
		ui_->cnvs->setItem(row, 5, new QTableWidgetItem(cn));
		//z scores
		QList<double> zscores = cnv.zScores();
		QString zs = "";
		for(int i=0;i<zscores.count();++i)
		{
			zs += QString::number(zscores[i]);
			if(i<zscores.count()-1) zs += ", ";
		}
		ui_->cnvs->setItem(row,6,new QTableWidgetItem(zs));

		QString genes = "";
		GeneSet gene_list = cnv.genes();
		for(int i=0;i<gene_list.count();++i)
		{
			genes += gene_list[i];
			if(i<gene_list.count()-1) genes += ", ";
		}
		ui_->cnvs->setItem(row,7,new QTableWidgetItem(genes));
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

CnvList SomaticReportConfiguration::getFilteredVariants()
{
	CnvList filtered_cnvs;

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

bool SomaticReportConfiguration::preselectVariant(const CopyNumberVariant& variant)
{
	bool preselect = true;

	//Prepare array with z-scores
	QList<double> zscores = variant.zScores();
	for(int j=0;j<zscores.count();++j)
	{
		zscores[j] = fabs(zscores[j]);
	}
	//only keep genes with high enough z-scores
	if(*std::max_element(zscores.begin(),zscores.end()) < 5.)
	{
		preselect = false;
	}
	if(zscores.length()< 10.)
	{
		preselect = false;
	}

	foreach (QByteArray gene, variant.genes())
	{
		if(keep_genes_cnv_.contains(gene) && fabs(*std::max_element(zscores.begin(),zscores.end()))>=5.)
		{
			preselect = true;
		}
	}
	if(preselect) return true;
	return false;
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
