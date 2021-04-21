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
#include <QMessageBox>
#include <QFileInfo>
#include <QBitArray>
#include <QClipboard>
#include <QMenu>
#include <QDesktopServices>
#include <QUrl>

RohWidget::RohWidget(QString filename, FilterWidget* filter_widget, QWidget *parent)
	: QWidget(parent)
	, var_filters(filter_widget)
	, ui(new Ui::RohWidget)
	, rohs()
{
	ui->setupUi(this);
	connect(ui->rohs, SIGNAL(itemDoubleClicked(QTableWidgetItem*)), this, SLOT(rohDoubleClicked(QTableWidgetItem*)));
	connect(ui->rohs, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(showContextMenu(QPoint)));
	connect(ui->copy_clipboard, SIGNAL(clicked(bool)), this, SLOT(copyToClipboard()));

	//connect
	connect(ui->f_roi, SIGNAL(stateChanged(int)), this, SLOT(filtersChanged()));
	connect(ui->f_genes, SIGNAL(stateChanged(int)), this, SLOT(filtersChanged()));
	connect(ui->f_size_kb, SIGNAL(valueChanged(double)), this, SLOT(filtersChanged()));
	connect(ui->f_size_markers, SIGNAL(valueChanged(int)), this, SLOT(filtersChanged()));
	connect(ui->f_q_score, SIGNAL(valueChanged(double)), this, SLOT(filtersChanged()));
	connect(ui->f_anno_name, SIGNAL(currentIndexChanged(int)), this, SLOT(filtersChanged()));
	connect(ui->f_anno_name, SIGNAL(currentIndexChanged(int)), this, SLOT(annotationFilterColumnChanged()));
	connect(ui->f_anno_op, SIGNAL(currentIndexChanged(int)), this, SLOT(filtersChanged()));
	connect(ui->f_anno_op, SIGNAL(currentIndexChanged(int)), this, SLOT(annotationFilterOperationChanged()));
	connect(ui->f_anno_value, SIGNAL(textEdited(QString)), this, SLOT(filtersChanged()));
	connect(var_filters, SIGNAL(filtersChanged()), this, SLOT(variantFiltersChanged()));
	connect(var_filters, SIGNAL(targetRegionChanged()), this, SLOT(variantFiltersChanged()));

	//load ROHs
	loadROHs(filename);

	//update variant list dependent filters (and apply filters)
	variantFiltersChanged();
}

RohWidget::~RohWidget()
{
	delete ui;
}

void RohWidget::rohDoubleClicked(QTableWidgetItem* item)
{
	if (item==nullptr) return;

	int col = item->column();
	int row = item->row();

	QString col_name = item->tableWidget()->horizontalHeaderItem(col)->text();
	if (col_name=="omim")
	{
		int omim_index = rohs.annotationHeaders().indexOf("omim");

		QString text = rohs[row].annotations()[omim_index].trimmed();
		if (text.trimmed()!="")
		{
			text = text.replace('_', ' ');
			text = text.replace("%3D", "=");
			VariantDetailsDockWidget::showOverviewTable("OMIM entries of ROH " +  rohs[row].toString(), text, ',', "https://www.omim.org/entry/");
		}
	}
	else
	{
		emit openRegionInIGV(rohs[item->row()].toString());
	}
}

void RohWidget::addInfoLine(QString text)
{
	ui->info_box->layout()->addWidget(new QLabel(text));
}

void RohWidget::disableGUI()
{
	ui->rohs->setEnabled(false);
	ui->f_genes->setEnabled(false);
	ui->f_roi->setEnabled(false);
	ui->f_size_kb->setEnabled(false);
	ui->f_size_markers->setEnabled(false);
	ui->f_q_score->setEnabled(false);
}

void RohWidget::loadROHs(QString filename)
{
	//load variants from file
	rohs.load(filename);

	//add generic annotations
	QVector<int> annotation_indices;
	for(int i=0; i<rohs.annotationHeaders().count(); ++i)
	{
		QByteArray header = rohs.annotationHeaders()[i];

		if (header=="size" || header=="region_count") continue;

		QTableWidgetItem* item = new QTableWidgetItem(QString(header));

		if (header=="omim")
		{
			item->setIcon(QIcon("://Icons/Table.png"));
			item->setToolTip("Double click table cell to open table view of annotations");
		}

		ui->rohs->setColumnCount(ui->rohs->columnCount() + 1);
		ui->rohs->setHorizontalHeaderItem(ui->rohs->columnCount() -1, item);
		annotation_indices.append(i);
	}

	//add generic annotation filter names
	ui->f_anno_name->clear();
	ui->f_anno_name->addItem("");
	foreach(int index, annotation_indices)
	{
		ui->f_anno_name->addItem(rohs.annotationHeaders()[index], index);
	}

	//show variants
	ui->rohs->setRowCount(rohs.count());
	for (int r=0; r<rohs.count(); ++r)
	{
		ui->rohs->setItem(r, 0, new QTableWidgetItem(rohs[r].toString()));
		QTableWidgetItem* item = new QTableWidgetItem(QString(rohs[r].genes().join(',')));
		GSvarHelper::colorGeneItem(item, rohs[r].genes());
		ui->rohs->setItem(r, 1, item);
		ui->rohs->setItem(r, 2, new QTableWidgetItem(QString::number(rohs[r].size()/1000.0, 'f', 3)));
		ui->rohs->setItem(r, 3, new QTableWidgetItem(QString::number(rohs[r].markerCount())));
		ui->rohs->setItem(r, 4, new QTableWidgetItem(QString::number(rohs[r].MarkerCountHet())));
		ui->rohs->setItem(r, 5, new QTableWidgetItem(QString::number(rohs[r].qScore())));

		int c = 6;
		foreach(int index, annotation_indices)
		{
			QTableWidgetItem* item = new QTableWidgetItem(QString(rohs[r].annotations()[index]));
			//special handling for OMIM
			if (rohs.annotationHeaders()[index]=="omim")
			{
				QString text = item->text();
				text = text.replace('_', ' ');
				text = text.replace("%3D", "=");
				item->setText(text);
			}
			item->setToolTip(item->text());
			ui->rohs->setItem(r, c++, item);
		}
	}

	//resize columns
	GUIHelper::resizeTableCells(ui->rohs, 200);
}

void RohWidget::filtersChanged()
{
	//init
	QBitArray pass;
	const int rows = rohs.count();
	pass.fill(true, rows);

	//filter by size (Kb)
	const double f_size_kb = 1000.0 * ui->f_size_kb->value();
	if (f_size_kb>0.0)
	{
		for(int r=0; r<rows; ++r)
		{
			if (!pass[r]) continue;
			pass[r] = rohs[r].size() >= f_size_kb;
		}
	}

	//filter by size (markers)
	const int f_size_markers = ui->f_size_markers->value();
	if (f_size_markers>0)
	{
		for(int r=0; r<rows; ++r)
		{
			if (!pass[r]) continue;
			pass[r] = rohs[r].markerCount() >= f_size_markers;
		}
	}

	//filter by Q score
	const double f_q_score = ui->f_q_score->value();
	if (f_q_score>0.0)
	{
		for(int r=0; r<rows; ++r)
		{
			if (!pass[r]) continue;
			pass[r] = rohs[r].qScore() >= f_q_score;
		}
	}

	//filter by genes
	if (ui->f_genes->isChecked())
	{
		for(int r=0; r<rows; ++r)
		{
			if (!pass[r]) continue;

			pass[r] = rohs[r].genes().intersectsWith(var_filters->genes());
		}
	}

	//filter by ROI
	if (ui->f_roi->isChecked())
	{
		const BedFile& roi =  var_filters->targetRegion().regions;

		for(int r=0; r<rows; ++r)
		{
			if (!pass[r]) continue;

			pass[r] = roi.overlapsWith(rohs[r].chr(), rohs[r].start(), rohs[r].end());
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
				pass[r] = rohs[r].annotations()[anno_col_index].isEmpty();
			}
		}
		else if (anno_op=="is not empty")
		{
			for(int r=0; r<rows; ++r)
			{
				if (!pass[r]) continue;
				pass[r] = !rohs[r].annotations()[anno_col_index].isEmpty();
			}
		}
		else
		{
			QByteArray anno_value = ui->f_anno_value->text().toLatin1().toUpper();
			for(int r=0; r<rows; ++r)
			{
				if (!pass[r]) continue;
				pass[r] = rohs[r].annotations()[anno_col_index].toUpper().contains(anno_value);
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
			ui->rohs->setRowHidden(r, false);
			++count_pass;
			size += rohs[r].size();
		}
		else
		{
			ui->rohs->setRowHidden(r, true);
		}
	}
	updateStatus(count_pass, size);
}

void RohWidget::variantFiltersChanged()
{
	ui->f_genes->setEnabled(!var_filters->genes().isEmpty());
	if (!ui->f_genes->isEnabled()) ui->f_genes->setChecked(false);

	ui->f_roi->setEnabled(var_filters->targetRegion().isValid());
	if (!ui->f_roi->isEnabled()) ui->f_roi->setChecked(false);

	//re-apply filters in case the genes/target region changed
	filtersChanged();
}

void RohWidget::copyToClipboard()
{
	GUIHelper::copyToClipboard(ui->rohs);
}

void RohWidget::annotationFilterColumnChanged()
{
	ui->f_anno_op->setEnabled(ui->f_anno_name->currentText()!="");
}

void RohWidget::annotationFilterOperationChanged()
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

void RohWidget::showContextMenu(QPoint p)
{
	//make sure a row was clicked
	int row = ui->rohs->indexAt(p).row();
	if (row==-1) return;

	//create menu
	QMenu menu;
	menu.addAction(QIcon("://Icons/UCSC.png"), "Open in UCSC browser");
	menu.addAction(QIcon("://Icons/UCSC.png"), "Open in UCSC browser (override tracks)");
	menu.addAction("Use as variant filter region");

	//exec menu
	QAction* action = menu.exec(ui->rohs->viewport()->mapToGlobal(p));
	if (action==nullptr) return;
	QString text = action->text();

	//UCSC
	if (text=="Open in UCSC browser")
	{
		QDesktopServices::openUrl(QUrl("https://genome.ucsc.edu/cgi-bin/hgTracks?db=hg19&position=" + rohs[row].toString()));
	}
	else if (text=="Open in UCSC browser (override tracks)")
	{
		QDesktopServices::openUrl(QUrl("https://genome.ucsc.edu/cgi-bin/hgTracks?db=hg19&ignoreCookie=1&hideTracks=1&cytoBand=pack&refSeqComposite=dense&ensGene=dense&omimGene2=pack&geneReviews=pack&dgvPlus=squish&genomicSuperDups=squish&position=" + rohs[row].toString()));
	}

	//Region filter
	if (text.startsWith("Use as variant filter region"))
	{
		var_filters->setRegion(rohs[row].toString());
	}
}

void RohWidget::updateStatus(int shown, double size)
{
	QString text = QString::number(shown) + "/" + QString::number(rohs.count()) + " passing filter(s) - size sum " + QString::number(size/1000000.0, 'f', 3) + " Mb";
	ui->status->setText(text);
}


