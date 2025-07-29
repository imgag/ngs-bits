#include "MethylationWidget.h"
#include "ui_MethylationWidget.h"

#include <FileLocation.h>
#include <GUIHelper.h>
#include <Helper.h>
#include <QDir>
#include <QMessageBox>
#include "GlobalServiceProvider.h"

MethylationWidget::MethylationWidget(QString filename, QWidget *parent) :
	QWidget(parent),
	ui_(new Ui::MethylationWidget),
	filename_(filename)
{
	ui_->setupUi(this);

	connect(ui_->tw_methylation, SIGNAL(cellDoubleClicked(int,int)), this, SLOT(openMethylationPlot(int, int)));
	connect(ui_->cb_include_REs, SIGNAL(stateChanged(int)), this, SLOT(updateTable()));
	connect(ui_->cb_include_promotors, SIGNAL(stateChanged(int)), this, SLOT(updateTable()));

	loadFile();
}

MethylationWidget::~MethylationWidget()
{
	delete ui_;
}

void MethylationWidget::loadFile()
{
	data_.load(filename_);

	updateTable();
}

void MethylationWidget::openMethylationPlot(int row_idx, int)
{
	QString locus = ui_->tw_methylation->verticalHeaderItem(row_idx)->data(Qt::UserRole).toString();

	FileLocation methylation_plot = GlobalServiceProvider::fileLocationProvider().getMethylationImage(locus);

	if (!methylation_plot.exists)
	{
		QMessageBox::warning(this, "File not found", "Could not find methylation plot image file: '" + methylation_plot.filename);
		return;
	}

	VersatileFile file(methylation_plot.filename);
	if (!file.open(QIODevice::ReadOnly, false))
	{
		QMessageBox::warning(this, "Read error", "Could not open a methylation plot image file: '" + methylation_plot.filename);
		return;
	}
	QPixmap plot_data;
	plot_data.loadFromData(file.readAll());

	QLabel* image_label = new QLabel();
	image_label->setPixmap(plot_data);

	//get sample name
	QString basename = QFileInfo(filename_).baseName();
	QString ps_name = basename.split("_").at(0) + "_" + basename.split("_").at(1);

	QSharedPointer<QDialog> dlg = GUIHelper::createDialog(image_label, "Methylation plot of " + locus + " (Processed Sample " + ps_name + ")");
	dlg->exec();

}

void MethylationWidget::updateTable()
{
	// fill columns
	ui_->tw_methylation->setColumnCount(15);
	int col_idx = 0;

	ui_->tw_methylation->setHorizontalHeaderItem(col_idx++, new QTableWidgetItem("title"));
	ui_->tw_methylation->setHorizontalHeaderItem(col_idx++, new QTableWidgetItem("gene"));
	ui_->tw_methylation->setHorizontalHeaderItem(col_idx++, new QTableWidgetItem("region"));
	ui_->tw_methylation->setHorizontalHeaderItem(col_idx++, new QTableWidgetItem("mean methylation all"));
	ui_->tw_methylation->setHorizontalHeaderItem(col_idx++, new QTableWidgetItem("stddev methylation all"));
	ui_->tw_methylation->setHorizontalHeaderItem(col_idx++, new QTableWidgetItem("mean methylation allele 1"));
	ui_->tw_methylation->setHorizontalHeaderItem(col_idx++, new QTableWidgetItem("stddev methylation allele 1"));
	ui_->tw_methylation->setHorizontalHeaderItem(col_idx++, new QTableWidgetItem("mean methylation allele 2"));
	ui_->tw_methylation->setHorizontalHeaderItem(col_idx++, new QTableWidgetItem("stddev methylation allele 2"));
	ui_->tw_methylation->setHorizontalHeaderItem(col_idx++, new QTableWidgetItem("mean methylation unphased"));
	ui_->tw_methylation->setHorizontalHeaderItem(col_idx++, new QTableWidgetItem("stddev methylation unphased"));
	ui_->tw_methylation->setHorizontalHeaderItem(col_idx++, new QTableWidgetItem("coverage all"));
	ui_->tw_methylation->setHorizontalHeaderItem(col_idx++, new QTableWidgetItem("coverage allele 1"));
	ui_->tw_methylation->setHorizontalHeaderItem(col_idx++, new QTableWidgetItem("coverage allele 2"));
	ui_->tw_methylation->setHorizontalHeaderItem(col_idx++, new QTableWidgetItem("coverage unphased"));

	//fill rows
	ui_->tw_methylation->setRowCount(data_.count());
	int row_idx = 0;
	for (int line_idx = 0; line_idx < data_.count(); ++line_idx)
	{
		const QStringList& line = data_[line_idx];
		col_idx = 0;

		//filter table:
		if ((ui_->cb_include_REs->checkState() != Qt::Checked) && line.at(0).startsWith("RepeatExpansion_")) continue;
		if ((ui_->cb_include_promotors->checkState() != Qt::Checked) && line.at(0).startsWith("Promotor_")) continue;

		//store identifier in header item
		QTableWidgetItem* v_header_item = new QTableWidgetItem("");
		v_header_item->setData(Qt::UserRole, line.at(0).trimmed());
		ui_->tw_methylation->setVerticalHeaderItem(row_idx, v_header_item);

		ui_->tw_methylation->setItem(row_idx, col_idx++, GUIHelper::createTableItem(line.at(1))); //title
		ui_->tw_methylation->setItem(row_idx, col_idx++, GUIHelper::createTableItem(line.at(2))); //gene
		QString region = line.at(3) + ":" + line.at(6) + "-" + line.at(7);
		ui_->tw_methylation->setItem(row_idx, col_idx++, GUIHelper::createTableItem(region));

		for (int i = 8; i < line.size(); ++i)
		{
			double value = Helper::toDouble(line.at(i), "TSV column " + QString::number(i), QString::number(row_idx));
			ui_->tw_methylation->setItem(row_idx, i-5, GUIHelper::createTableItem(value, 2));
		}
		row_idx++;
	}
	//set actual row size
	ui_->tw_methylation->setRowCount(row_idx);

	GUIHelper::resizeTableCellWidths(ui_->tw_methylation);
	ui_->tw_methylation->setAlternatingRowColors(true);
}
