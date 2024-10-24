#include "RohWidget.h"
#include "ui_RohWidget.h"
#include "Helper.h"
#include "Exceptions.h"
#include "GUIHelper.h"
#include "VariantDetailsDockWidget.h"
#include "NGSD.h"
#include "Settings.h"
#include "GSvarHelper.h"
#include "GlobalServiceProvider.h"
#include "IgvSessionManager.h"
#include <QMessageBox>
#include <QFileInfo>
#include <QBitArray>
#include <QClipboard>
#include <QMenu>
#include <QDesktopServices>
#include <QUrl>

RohWidget::RohWidget(QWidget* parent, QString filename)
	: QWidget(parent)
	, ui_(new Ui::RohWidget)
	, rohs_()
	, var_filters_(GlobalServiceProvider::filterWidget())
{
	ui_->setupUi(this);
	connect(ui_->rohs, SIGNAL(itemDoubleClicked(QTableWidgetItem*)), this, SLOT(rohDoubleClicked(QTableWidgetItem*)));
	connect(ui_->rohs, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(showContextMenu(QPoint)));
	connect(ui_->copy_clipboard, SIGNAL(clicked(bool)), this, SLOT(copyToClipboard()));

	//connect
	connect(ui_->f_roi, SIGNAL(stateChanged(int)), this, SLOT(filtersChanged()));
	connect(ui_->f_genes, SIGNAL(stateChanged(int)), this, SLOT(filtersChanged()));
	connect(ui_->f_size_kb, SIGNAL(valueChanged(double)), this, SLOT(filtersChanged()));
	connect(ui_->f_size_markers, SIGNAL(valueChanged(int)), this, SLOT(filtersChanged()));
	connect(ui_->f_q_score, SIGNAL(valueChanged(double)), this, SLOT(filtersChanged()));
	connect(ui_->f_anno_name, SIGNAL(currentIndexChanged(int)), this, SLOT(filtersChanged()));
	connect(ui_->f_anno_name, SIGNAL(currentIndexChanged(int)), this, SLOT(annotationFilterColumnChanged()));
	connect(ui_->f_anno_op, SIGNAL(currentIndexChanged(int)), this, SLOT(filtersChanged()));
	connect(ui_->f_anno_op, SIGNAL(currentIndexChanged(int)), this, SLOT(annotationFilterOperationChanged()));
	connect(ui_->f_anno_value, SIGNAL(textEdited(QString)), this, SLOT(filtersChanged()));
	connect(var_filters_, SIGNAL(filtersChanged()), this, SLOT(variantFiltersChanged()));
	connect(var_filters_, SIGNAL(targetRegionChanged()), this, SLOT(variantFiltersChanged()));

	//load ROHs
	loadROHs(filename);

	//update variant list dependent filters (and apply filters)
	variantFiltersChanged();
}

RohWidget::~RohWidget()
{
	delete ui_;
}

void RohWidget::rohDoubleClicked(QTableWidgetItem* item)
{
	if (item==nullptr) return;

	int col = item->column();
	int row = item->row();

	QString col_name = item->tableWidget()->horizontalHeaderItem(col)->text();
	if (col_name=="omim")
	{
		int omim_index = rohs_.annotationHeaders().indexOf("omim");

		QString text = rohs_[row].annotations()[omim_index].trimmed();
		if (text.trimmed()!="")
		{
			text = text.replace('_', ' ');
			text = text.replace("%3D", "=");
			VariantDetailsDockWidget::showOverviewTable("OMIM entries of ROH " +  rohs_[row].toString(), text, ',', "https://www.omim.org/entry/");
		}
	}
	else
	{
		IgvSessionManager::get(0).gotoInIGV(rohs_[item->row()].toString(), true);
	}
}

void RohWidget::addInfoLine(QString text)
{
	ui_->info_box->layout()->addWidget(new QLabel(text));
}

void RohWidget::disableGUI()
{
	ui_->rohs->setEnabled(false);
	ui_->f_genes->setEnabled(false);
	ui_->f_roi->setEnabled(false);
	ui_->f_size_kb->setEnabled(false);
	ui_->f_size_markers->setEnabled(false);
	ui_->f_q_score->setEnabled(false);
}

void RohWidget::loadROHs(QString filename)
{
	//load variants from file
	rohs_.load(filename);

	//add generic annotations
	QVector<int> annotation_indices;
	for(int i=0; i<rohs_.annotationHeaders().count(); ++i)
	{
		QByteArray header = rohs_.annotationHeaders()[i];

		if (header=="size" || header=="region_count") continue;

		QTableWidgetItem* item = new QTableWidgetItem(QString(header));

		if (header=="omim")
		{
			item->setIcon(QIcon("://Icons/Table.png"));
			item->setToolTip("Double click table cell to open table view of annotations");
		}

		ui_->rohs->setColumnCount(ui_->rohs->columnCount() + 1);
		ui_->rohs->setHorizontalHeaderItem(ui_->rohs->columnCount() -1, item);
		annotation_indices.append(i);
	}

	//add generic annotation filter names
	ui_->f_anno_name->clear();
	ui_->f_anno_name->addItem("");
	foreach(int index, annotation_indices)
	{
		ui_->f_anno_name->addItem(rohs_.annotationHeaders()[index], index);
	}

	//show variants
	ui_->rohs->setRowCount(rohs_.count());
	for (int r=0; r<rohs_.count(); ++r)
	{
		ui_->rohs->setItem(r, 0, new QTableWidgetItem(rohs_[r].toString()));
		QTableWidgetItem* item = new QTableWidgetItem(QString(rohs_[r].genes().join(',')));
		GSvarHelper::colorGeneItem(item, rohs_[r].genes());
		ui_->rohs->setItem(r, 1, item);
		ui_->rohs->setItem(r, 2, new QTableWidgetItem(QString::number(rohs_[r].size()/1000.0, 'f', 3)));
		ui_->rohs->setItem(r, 3, new QTableWidgetItem(QString::number(rohs_[r].markerCount())));
		ui_->rohs->setItem(r, 4, new QTableWidgetItem(QString::number(rohs_[r].MarkerCountHet())));
		ui_->rohs->setItem(r, 5, new QTableWidgetItem(QString::number(rohs_[r].qScore())));

		int c = 6;
		foreach(int index, annotation_indices)
		{
			QTableWidgetItem* item = new QTableWidgetItem(QString(rohs_[r].annotations()[index]));
			//special handling for OMIM
			if (rohs_.annotationHeaders()[index]=="omim")
			{
				QString text = item->text();
				text = text.replace('_', ' ');
				text = text.replace("%3D", "=");
				item->setText(text);
			}
			item->setToolTip(item->text());
			ui_->rohs->setItem(r, c++, item);
		}
	}

	//resize columns
	GUIHelper::resizeTableCellWidths(ui_->rohs, 200);
	GUIHelper::resizeTableCellHeightsToFirst(ui_->rohs);
}

void RohWidget::filtersChanged()
{
	//init
	QBitArray pass;
	const int rows = rohs_.count();
	pass.fill(true, rows);

	//filter by size (Kb)
	const double f_size_kb = 1000.0 * ui_->f_size_kb->value();
	if (f_size_kb>0.0)
	{
		for(int r=0; r<rows; ++r)
		{
			if (!pass[r]) continue;
			pass[r] = rohs_[r].size() >= f_size_kb;
		}
	}

	//filter by size (markers)
	const int f_size_markers = ui_->f_size_markers->value();
	if (f_size_markers>0)
	{
		for(int r=0; r<rows; ++r)
		{
			if (!pass[r]) continue;
			pass[r] = rohs_[r].markerCount() >= f_size_markers;
		}
	}

	//filter by Q score
	const double f_q_score = ui_->f_q_score->value();
	if (f_q_score>0.0)
	{
		for(int r=0; r<rows; ++r)
		{
			if (!pass[r]) continue;
			pass[r] = rohs_[r].qScore() >= f_q_score;
		}
	}

	//filter by genes
	if (ui_->f_genes->isChecked())
	{
		for(int r=0; r<rows; ++r)
		{
			if (!pass[r]) continue;

			pass[r] = rohs_[r].genes().intersectsWith(var_filters_->genes());
		}
	}

	//filter by ROI
	if (ui_->f_roi->isChecked())
	{
		const BedFile& roi =  var_filters_->targetRegion().regions;

		for(int r=0; r<rows; ++r)
		{
			if (!pass[r]) continue;

			pass[r] = roi.overlapsWith(rohs_[r].chr(), rohs_[r].start(), rohs_[r].end());
		}
	}

	//filter by generic annotation
	if (ui_->f_anno_name->currentText()!="")
	{
		int anno_col_index = ui_->f_anno_name->currentData().toInt();
		QString anno_op = ui_->f_anno_op->currentText();
		if (anno_op=="is empty")
		{
			for(int r=0; r<rows; ++r)
			{
				if (!pass[r]) continue;
				pass[r] = rohs_[r].annotations()[anno_col_index].isEmpty();
			}
		}
		else if (anno_op=="is not empty")
		{
			for(int r=0; r<rows; ++r)
			{
				if (!pass[r]) continue;
				pass[r] = !rohs_[r].annotations()[anno_col_index].isEmpty();
			}
		}
		else
		{
			QByteArray anno_value = ui_->f_anno_value->text().toUtf8().toUpper();
			for(int r=0; r<rows; ++r)
			{
				if (!pass[r]) continue;
				pass[r] = rohs_[r].annotations()[anno_col_index].toUpper().contains(anno_value);
			}
		}
	}

	//update GUI
	double count_pass = 0;
	double size = 0.0;
	for(int r=0; r<rows; ++r)
	{
		if (pass[r])
		{
			ui_->rohs->setRowHidden(r, false);
			++count_pass;
			size += rohs_[r].size();
		}
		else
		{
			ui_->rohs->setRowHidden(r, true);
		}
	}
	updateStatus(count_pass, size);
}

void RohWidget::variantFiltersChanged()
{
	ui_->f_genes->setEnabled(!var_filters_->genes().isEmpty());
	if (!ui_->f_genes->isEnabled()) ui_->f_genes->setChecked(false);

	ui_->f_roi->setEnabled(var_filters_->targetRegion().isValid());
	if (!ui_->f_roi->isEnabled()) ui_->f_roi->setChecked(false);

	//re-apply filters in case the genes/target region changed
	filtersChanged();
}

void RohWidget::copyToClipboard()
{
	GUIHelper::copyToClipboard(ui_->rohs);
}

void RohWidget::annotationFilterColumnChanged()
{
	ui_->f_anno_op->setEnabled(ui_->f_anno_name->currentText()!="");
}

void RohWidget::annotationFilterOperationChanged()
{
	if (ui_->f_anno_op->currentText()=="contains")
	{
		ui_->f_anno_value->setEnabled(true);
	}
	else
	{
		ui_->f_anno_value->setEnabled(false);
		ui_->f_anno_value->clear();
	}
}

void RohWidget::showContextMenu(QPoint p)
{
	//make sure a row was clicked
	int row = ui_->rohs->indexAt(p).row();
	if (row==-1) return;

	//create menu
	QMenu menu;
	menu.addAction(QIcon("://Icons/UCSC.png"), "Open in UCSC browser");
	menu.addAction(QIcon("://Icons/UCSC.png"), "Open in UCSC browser (override tracks)");
	menu.addAction("Use as variant filter region");

	//exec menu
	QAction* action = menu.exec(ui_->rohs->viewport()->mapToGlobal(p));
	if (action==nullptr) return;
	QString text = action->text();

	//UCSC
	if (text=="Open in UCSC browser")
	{
		QDesktopServices::openUrl(QUrl("https://genome.ucsc.edu/cgi-bin/hgTracks?db="+buildToString(GSvarHelper::build())+"&position=" + rohs_[row].toString()));
	}
	else if (text=="Open in UCSC browser (override tracks)")
	{
		QDesktopServices::openUrl(QUrl("https://genome.ucsc.edu/cgi-bin/hgTracks?db="+buildToString(GSvarHelper::build())+"&ignoreCookie=1&hideTracks=1&cytoBand=pack&refSeqComposite=dense&ensGene=dense&omimGene2=pack&geneReviews=pack&dgvPlus=squish&genomicSuperDups=squish&position=" + rohs_[row].toString()));
	}

	//Region filter
	if (text.startsWith("Use as variant filter region"))
	{
		var_filters_->setRegion(rohs_[row].toString());
	}
}

void RohWidget::updateStatus(int shown, double size)
{
	QString text = QString::number(shown) + "/" + QString::number(rohs_.count()) + " passing filter(s) - size sum " + QString::number(size/1000000.0, 'f', 3) + " Mb";
	ui_->status->setText(text);
}


