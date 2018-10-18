#include "SomaticReportConfiguration.h"
#include "ui_SomaticReportConfiguration.h"
#include "GUIHelper.h"
#include <QMenu>
#include <QBitArray>
#include "cmath"

SomaticReportConfiguration::SomaticReportConfiguration(const ClinCnvList& cnv_input, QWidget *parent)
	: QDialog(parent)
	, ui_(new Ui::SomaticReportConfiguration)
	, cnvs_(cnv_input)
{
	ui_->setupUi(this);
	view_pass_filter.fill(true,cnvs_.count());

	//set up CNV table
	QStringList col_names;
	col_names << "" <<  "chr" << "start" << "end" << "copy number" <<  "log likelihood" << "size" << "genes";

	//annotations
	for(int i=0;i<cnvs_.annotationHeaders().count();++i)
	{
		col_names << cnvs_.annotationHeaders().at(i);
	}

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
		ui_->cnvs->item(row, 4)->setTextAlignment(Qt::AlignRight|Qt::AlignVCenter);
		ui_->cnvs->setItem(row, 5, new QTableWidgetItem(QString::number(cnv.likelihood(), 'f', 1)));
		ui_->cnvs->item(row, 5)->setTextAlignment(Qt::AlignRight|Qt::AlignVCenter);
		ui_->cnvs->setItem(row, 6, new QTableWidgetItem(QString::number(cnv.size(),'f',0)));
		ui_->cnvs->item(row, 6)->setTextAlignment(Qt::AlignRight|Qt::AlignVCenter);
		ui_->cnvs->setItem(row, 7, new QTableWidgetItem(QString(cnv.genes().join(", "))));

		//additional annotations
		for(int i=0;i<cnv.annotations().count();++i)
		{
			ui_->cnvs->setItem(row,8+i, new QTableWidgetItem(QString::fromLatin1(cnv.annotations().at(i))));
		}
	}
	GUIHelper::resizeTableCells(ui_->cnvs, 600);


	/************************
	 * DISPLAY SAMPLE INFOS *
	 ************************/
	QByteArray cgi_cancer_type = "";
	foreach(QByteArray comment,cnvs_.comments())
	{
		if(comment.contains("CGI_CANCER_TYPE"))
		{
			cgi_cancer_type = comment.split('=')[1];
			break;
		}
	}
	if(cgi_cancer_type == "") cgi_cancer_type = "UNKNOWN";

	if(cgi_cancer_type == "UNKNOWN" || cgi_cancer_type == "CANCER")
	{
		ui_->infos->layout()->addWidget(new QLabel("<font color='red'>CGI cancer type: " + cgi_cancer_type +"</font>"));
	}
	else
	{
		ui_->infos->layout()->addWidget(new QLabel("CGI cancer type: " + cgi_cancer_type));
	}

	if(!cnvs_.annotationHeaders().contains("CGI_driver_statement"))
	{
		ui_->filter_for_cgi_drivers->hide();
	}

	/*********************
	 * SLOTS AND SIGNALS *
	 *********************/
	//buttons
	connect(ui_->buttonBox,SIGNAL(accepted()),this,SLOT(accept()));
	connect(ui_->buttonBox,SIGNAL(rejected()),this,SLOT(reject()));

	//context menu
	ui_->cnvs->setContextMenuPolicy(Qt::CustomContextMenu);
	connect(ui_->cnvs, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(showContextMenu(QPoint)));

	//Update filters
	connect(ui_->log_likelihood,SIGNAL(valueChanged(double)),this,SLOT(filtersChanged()));
	connect(ui_->cnv_min_size,SIGNAL(valueChanged(double)),this,SLOT(filtersChanged()));
	connect(ui_->filter_for_cgi_drivers,SIGNAL(stateChanged(int)),this,SLOT(filtersChanged()));
	connect(ui_->deselect_x_y,SIGNAL(clicked(bool)),this,SLOT(deselectXY()));

	//reset viewing filter
	connect(ui_->reset_view_filters,SIGNAL(clicked(bool)),this,SLOT(resetView()));

	//apply viewing filter to preselection
	connect(ui_->preselect_cnvs,SIGNAL(clicked(bool)),this,SLOT(selectCNVsFromView()));

	//open in IGV
	connect(ui_->cnvs,SIGNAL(itemDoubleClicked(QTableWidgetItem*)),this,SLOT(cnvDoubleClicked(QTableWidgetItem*)));

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

void SomaticReportConfiguration::filtersChanged()
{
	double min_loglikelihood = ui_->log_likelihood->value();
	double min_cnv_size = ui_->cnv_min_size->value();

	view_pass_filter.fill(true);

	//Logarithmic likelihood
	for(int r=0;r<cnvs_.count();++r)
	{
		if(!view_pass_filter[r]) continue;
		view_pass_filter[r] = cnvs_[r].likelihood() >= min_loglikelihood;
	}

	//CNV size
	for(int r=0;r<cnvs_.count();++r)
	{
		if(!view_pass_filter[r]) continue;
		view_pass_filter[r] = cnvs_[r].size() >= min_cnv_size * 1000;
	}

	//Filter for CGI driver
	if(ui_->filter_for_cgi_drivers->checkState() == Qt::Checked)
	{
		int i_cgi_driver_statement = cnvs_.annotationIndexByName("CGI_driver_statement",false);

		if(i_cgi_driver_statement >= 0)
		{
			for(int r=0;r<cnvs_.count();++r)
			{
				if(!view_pass_filter[r]) continue;

				bool is_driver = false;
				QByteArrayList driver_statements = cnvs_[r].annotations().at(i_cgi_driver_statement).split(',');
				foreach(QByteArray driver_statement,driver_statements)
				{
					if(driver_statement.contains("driver") || driver_statement.contains("known"))
					{
						is_driver = true;
						break;
					}
				}
				view_pass_filter[r] = is_driver;
			}
		}
	}

	//Update GUI
	for(int r=0;r<cnvs_.count();++r)
	{
		ui_->cnvs->setRowHidden(r,!view_pass_filter[r]);
	}
}

void SomaticReportConfiguration::resetView()
{
	ui_->log_likelihood->setValue(0);
	ui_->cnv_min_size->setValue(0);
	ui_->filter_for_cgi_drivers->setCheckState(Qt::Unchecked);
}

void SomaticReportConfiguration::selectCNVsFromView()
{
	for(int r=0;r<ui_->cnvs->rowCount();++r)
	{
		if(view_pass_filter[r])
		{
			ui_->cnvs->item(r,0)->setCheckState(Qt::Checked);
		}
		else
		{
			ui_->cnvs->item(r,0)->setCheckState(Qt::Unchecked);
		}
	}
}

void SomaticReportConfiguration::deselectXY()
{
	for(int r=0;r<ui_->cnvs->rowCount();++r)
	{
		if(ui_->cnvs->item(r,1)->text().contains("Y") || ui_->cnvs->item(r,1)->text().contains("X"))
		{
			ui_->cnvs->item(r,0)->setCheckState(Qt::Unchecked);
		}
	}
}

void SomaticReportConfiguration::cnvDoubleClicked(QTableWidgetItem* item)
{
	if (item==nullptr) return;

	emit openRegionInIGV(cnvs_[item->row()].toString());
}
