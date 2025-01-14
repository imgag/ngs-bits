#include "MethylationWidget.h"
#include "ui_MethylationWidget.h"

#include <FileLocation.h>
#include <GUIHelper.h>
#include <Helper.h>
#include "GlobalServiceProvider.h"

MethylationWidget::MethylationWidget(QString filename, QWidget *parent) :
	QWidget(parent),
	ui_(new Ui::MethylationWidget),
	filename_(filename)
{
	ui_->setupUi(this);

	connect(ui_->tw_methylation, SIGNAL(cellDoubleClicked(int,int)), this, SLOT(openMethylationPlot(int, int)));

	loadFile();
}

MethylationWidget::~MethylationWidget()
{
	delete ui_;
}

void MethylationWidget::loadFile()
{
	data_.load(filename_);

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
	for (int row_idx = 0; row_idx < data_.count(); ++row_idx)
	{
		const QStringList& line = data_[row_idx];
		col_idx = 0;

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
	}

	GUIHelper::resizeTableCellWidths(ui_->tw_methylation);
	ui_->tw_methylation->setAlternatingRowColors(true);

}

void MethylationWidget::openMethylationPlot(int row_idx, int col_idx)
{
	QString locus = ui_->tw_methylation->verticalHeaderItem(row_idx)->data(Qt::UserRole).toString();

	FileLocation methylation_plot = GlobalServiceProvider::fileLocationProvider().getMethylationImage(locus);

	qDebug() << methylation_plot.filename;

	if (!methylation_plot.exists)
	{
		GUIHelper::showMessage("File not found!", "Methylation plot for '" + locus + "' not found!");
		return;
	}

	QPixmap plot_data(methylation_plot.filename);

	QLabel* image_label = new QLabel();
	image_label->setPixmap(plot_data);

	QSharedPointer<QDialog> dlg = GUIHelper::createDialog(image_label, "Methylation plot of " + locus);
	dlg->exec();
}
