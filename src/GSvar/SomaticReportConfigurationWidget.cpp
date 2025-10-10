#include "SomaticReportConfigurationWidget.h"
#include "ui_SomaticReportConfigurationWidget.h"
#include "GlobalServiceProvider.h"
#include "GUIHelper.h"
#include <QMenu>
#include <QApplication>
#include <QShortcut>
#include "cmath"

SomaticReportConfigurationWidget::SomaticReportConfigurationWidget(const CnvList& cnv_input, QWidget *parent)
	: QDialog(parent)
	, ui_(new Ui::SomaticReportConfigurationWidget)
	, cnvs_(cnv_input)
	, db_()
{
	ui_->setupUi(this);
	view_pass_filter.fill(true,cnvs_.count());

	fillCnvWidget();


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


	/*********************
	 * SLOTS AND SIGNALS *
	 *********************/
	//create shortcut (copy data to clipboard)
	new QShortcut(QKeySequence(Qt::CTRL + Qt::Key_C),this,SLOT(copyToClipboard()));
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

SomaticReportConfigurationWidget::~SomaticReportConfigurationWidget()
{
	delete ui_;
}

void SomaticReportConfigurationWidget::fillCnvWidget()
{
	if(cnvs_.count() == 0)
	{
		disableGUI();
		return;
	}

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

	int i_cn = cnvs_.annotationIndexByName("CN_change", true);
	int i_ll = cnvs_.annotationIndexByName("loglikelihood", true);

	//load data into QTableWidget cnvs
	ui_->cnvs->setRowCount(cnvs_.count());
	for(int row=0;row<cnvs_.count();++row)
	{
		const CopyNumberVariant& cnv = cnvs_[row];

		//checkbox
		ui_->cnvs->setItem(row, 0, new QTableWidgetItem(""));
		ui_->cnvs->item(row, 0)->setFlags(Qt::ItemIsUserCheckable|Qt::ItemIsEnabled);
		ui_->cnvs->item(row, 0)->setTextAlignment(Qt::AlignCenter);
		ui_->cnvs->item(row, 0)->setCheckState(Qt::Checked);

		//data
		ui_->cnvs->setItem(row, 1, new QTableWidgetItem(QString(cnv.chr().str())));
		ui_->cnvs->setItem(row, 2, new QTableWidgetItem(QString::number(cnv.start())));
		ui_->cnvs->setItem(row, 3, new QTableWidgetItem(QString::number(cnv.end())));
		ui_->cnvs->setItem(row, 4, new QTableWidgetItem(QString(cnv.annotations().at(i_cn))));
		ui_->cnvs->item(row, 4)->setTextAlignment(Qt::AlignRight|Qt::AlignVCenter);
		ui_->cnvs->setItem(row, 5, new QTableWidgetItem(QString(cnv.annotations().at(i_ll))));
		ui_->cnvs->item(row, 5)->setTextAlignment(Qt::AlignRight|Qt::AlignVCenter);
		ui_->cnvs->setItem(row, 6, new QTableWidgetItem(QString::number(cnv.size(),'f',0)));
		ui_->cnvs->item(row, 6)->setTextAlignment(Qt::AlignRight|Qt::AlignVCenter);
		ui_->cnvs->setItem(row, 7, new QTableWidgetItem(QString(cnv.genes().join(", "))));

		//additional annotations
		for(int i=0;i<cnv.annotations().count();++i)
		{
                        ui_->cnvs->setItem(row,8+i, new QTableWidgetItem(QString::fromUtf8(cnv.annotations().at(i))));
		}
	}
	GUIHelper::resizeTableCells(ui_->cnvs, 600);
}

void SomaticReportConfigurationWidget::setSelectionRnaDna(bool enabled)
{
	ui_->report_type_label->setEnabled(enabled);
	ui_->report_type_dna->setEnabled(enabled);
	ui_->report_type_rna->setEnabled(enabled);
}

SomaticReportConfigurationWidget::report_type SomaticReportConfigurationWidget::getReportType()
{
	if(ui_->report_type_dna->isChecked()) return report_type::DNA;
	else return report_type::RNA;
}

CnvList SomaticReportConfigurationWidget::getSelectedCNVs()
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


void SomaticReportConfigurationWidget::showContextMenu(QPoint pos)
{
	QMenu menu(ui_->cnvs);

	menu.addAction(QIcon(":/Icons/box_checked.png"), "select all");
	menu.addAction(QIcon(":/Icons/box_unchecked.png"), "unselect all");

	QAction* action = menu.exec(ui_->cnvs->viewport()->mapToGlobal(pos));
	if(action==nullptr) return;

	QByteArray text = action->text().toUtf8();
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

void SomaticReportConfigurationWidget::disableGUI()
{
	ui_->log_likelihood->setEnabled(false);
	ui_->cnvs->setEnabled(false);
	ui_->deselect_x_y->setEnabled(false);
	ui_->preselect_cnvs->setEnabled(false);
	ui_->reset_view_filters->setEnabled(false);
	ui_->filter_for_cgi_drivers->setEnabled(false);
	ui_->cnv_min_size->setEnabled(false);
}

void SomaticReportConfigurationWidget::filtersChanged()
{
	double min_loglikelihood = ui_->log_likelihood->value();
	double min_cnv_size = ui_->cnv_min_size->value();

	view_pass_filter.fill(true);

	//Logarithmic likelihood

	int i_ll = cnvs_.annotationIndexByName("loglikelihood", true);
	for(int r=0;r<cnvs_.count();++r)
	{
		if(!view_pass_filter[r]) continue;
		view_pass_filter[r] = cnvs_[r].annotations()[i_ll].toDouble() >= min_loglikelihood;
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
		int i_cgi_driver_statement = cnvs_.annotationIndexByName("CGI_driver_statement", false);

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

void SomaticReportConfigurationWidget::resetView()
{
	ui_->log_likelihood->setValue(0);
	ui_->cnv_min_size->setValue(0);
	ui_->filter_for_cgi_drivers->setCheckState(Qt::Unchecked);
}

void SomaticReportConfigurationWidget::selectCNVsFromView()
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

void SomaticReportConfigurationWidget::deselectXY()
{
	for(int r=0;r<ui_->cnvs->rowCount();++r)
	{
		if(ui_->cnvs->item(r,1)->text().contains("Y") || ui_->cnvs->item(r,1)->text().contains("X"))
		{
			ui_->cnvs->item(r,0)->setCheckState(Qt::Unchecked);
		}
	}
}

void SomaticReportConfigurationWidget::cnvDoubleClicked(QTableWidgetItem* item)
{
	if (item==nullptr) return;

        GlobalServiceProvider::gotoInIGV(cnvs_[item->row()].toString());
}

void SomaticReportConfigurationWidget::copyToClipboard()
{
	QTableWidgetSelectionRange range = ui_->cnvs->selectedRanges()[0];
	//copy header
	QString selected_text = "";
	if (range.rowCount()!=1)
	{
		selected_text += "#";
		for (int col=range.leftColumn(); col<=range.rightColumn(); ++col)
		{
			if (col!=range.leftColumn()) selected_text.append("\t");
			selected_text.append(ui_->cnvs->horizontalHeaderItem(col)->text());
		}
	}

	//copy rows
	for (int row=range.topRow(); row<=range.bottomRow(); ++row)
	{
		//skip filtered-out rows
		if (ui_->cnvs->isRowHidden(row)) continue;
		if (selected_text!="") selected_text.append("\n");
		for (int col=range.leftColumn(); col<=range.rightColumn(); ++col)
		{
			if (col!=range.leftColumn()) selected_text.append("\t");
			QTableWidgetItem* item = ui_->cnvs->item(row, col);
			if (item==nullptr) continue;
			selected_text.append(item->text().replace('\n',' ').replace('\r', ""));
		}
	}
	QApplication::clipboard()->setText(selected_text);
}


