#include "MethylationWidget.h"
#include "ui_MethylationWidget.h"

#include <FileLocation.h>
#include <GUIHelper.h>
#include <Helper.h>
#include <QMessageBox>
#include "GlobalServiceProvider.h"
#include "ImageLabel.h"

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
	FileLocation methylation_cohort_plot = GlobalServiceProvider::fileLocationProvider().getMethylationCohortImage(locus);

	//load methylartist plot
	if (!methylation_plot.exists)
	{
		QMessageBox::warning(this, "File not found", "Could not find methylation plot image file: '" + methylation_plot.filename + "'!");
		return;
	}
	VersatileFile file(methylation_plot.filename);
	if (!file.open(QIODevice::ReadOnly, false))
	{
		QMessageBox::warning(this, "Read error", "Could not open a methylation plot image file: '" + methylation_plot.filename + "'!");
		return;
	}
	QVBoxLayout* v_box = new QVBoxLayout();
	ImageLabel* image_label = new ImageLabel();
	QImage image = QImage(methylation_plot.filename);
	image_label->setImage(image);
	image_label->resize(1024, 489);
	v_box->addWidget(image_label);

	//load cohort plot
	ImageLabel* image_label_cohort_plot = new ImageLabel();
	if (methylation_cohort_plot.exists)
	{
		VersatileFile file(methylation_cohort_plot.filename);
		if (!file.open(QIODevice::ReadOnly, false))
		{
			QMessageBox::warning(this, "Read error", "Could not open a methylation cohort plot image file: '" + methylation_cohort_plot.filename);
			return;
		}

		QImage image = QImage(methylation_cohort_plot.filename);
		image_label_cohort_plot->setImage(image);
		image_label_cohort_plot->resize(1024, 489);
		v_box->addWidget(image_label_cohort_plot);
	}
	else
	{
		Log::warn("File not found:\tCould not find methylation cohort plot image file: '" + methylation_cohort_plot.filename + "'!");
	}

	//get sample name
	QString basename = QFileInfo(filename_).baseName();
	QString ps_name = basename.split("_").at(0) + "_" + basename.split("_").at(1);

	QWidget* widget = new QWidget();
	widget->setLayout(v_box);
	QSharedPointer<QDialog> dlg = GUIHelper::createDialog(widget, "Methylation plot of " + locus + " (Processed Sample " + ps_name + ")");
	dlg->exec();

}

void MethylationWidget::updateTable()
{
	//Checks:
	const QStringList header = data_.headers();
	//First 8 columns have to be fix
	if (header.at(0) != "identifier") THROW(FileParseException, "Invalid methylation file! First column has to be identifier!");
	if (header.at(1) != "title") THROW(FileParseException, "Invalid methylation file! First column has to be title!");
	if (header.at(2) != "gene symbol") THROW(FileParseException, "Invalid methylation file! First column has to be gene symbol!");
	if (header.at(3) != "gene chr") THROW(FileParseException, "Invalid methylation file! First column has to be gene chr!");
	if (header.at(4) != "gene start") THROW(FileParseException, "Invalid methylation file! First column has to be gene start!");
	if (header.at(5) != "gene end") THROW(FileParseException, "Invalid methylation file! First column has to be gene end!");
	if (header.at(6) != "highlight start") THROW(FileParseException, "Invalid methylation file! First column has to be highlight start!");
	if (header.at(7) != "highlight end") THROW(FileParseException, "Invalid methylation file! 8th column has to be highlight end!");

	QSet<QString> float_columns;
	float_columns << "meth_hp1_avg" << "meth_hp1_std" << "cov_hp1" << "meth_hp2_avg" << "meth_hp2_std" << "cov_hp2" << "meth_nohp_avg" << "meth_nohp_std" << "cov_nohp";
	float_columns << "cohort_mean_hp1" << "cohort_stdev_hp1" << "zscore_hp1" << "cohort_mean_hp2" << "cohort_stdev_hp2" << "zscore_hp2";
	float_columns << "meth_all_avg" << "meth_all_std" << "cov_all" << "cohort_mean_all" << "cohort_stdev_all" << "zscore_all";

	QSet<QString> int_columns;


	// fill columns
	ui_->tw_methylation->setColumnCount(data_.columnCount()-5); //first 8 columns will be replaced by 3 fixed columns (-8+3)=-5

	//init header
	int col_idx = 0;
	//fixed columns
	ui_->tw_methylation->setHorizontalHeaderItem(col_idx++, new QTableWidgetItem("title"));
	ui_->tw_methylation->setHorizontalHeaderItem(col_idx++, new QTableWidgetItem("gene"));
	ui_->tw_methylation->setHorizontalHeaderItem(col_idx++, new QTableWidgetItem("region"));
	//additional columns
	for (int header_idx = 8; header_idx < header.count(); ++header_idx)
	{
		ui_->tw_methylation->setHorizontalHeaderItem(col_idx++, new QTableWidgetItem(header.at(header_idx)));
	}

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

		//store fixed columns
		ui_->tw_methylation->setItem(row_idx, col_idx++, GUIHelper::createTableItem(line.at(1))); //title
		ui_->tw_methylation->setItem(row_idx, col_idx++, GUIHelper::createTableItem(line.at(2))); //gene
		QString region = line.at(3) + ":" + line.at(6) + "-" + line.at(7);
		ui_->tw_methylation->setItem(row_idx, col_idx++, GUIHelper::createTableItem(region));

		// fill rest of table
		for (int line_col_idx = 8; line_col_idx < line.count(); ++line_col_idx) //ignore first 8 columns
		{
			if (float_columns.contains(header.at(line_col_idx)))
			{
				ui_->tw_methylation->setItem(row_idx, col_idx, GUIHelper::createTableItem(line.at(line_col_idx).toDouble(), 3));
			}
			else if (int_columns.contains(header.at(line_col_idx)))
			{
				ui_->tw_methylation->setItem(row_idx, col_idx, GUIHelper::createTableItem(line.at(line_col_idx).toInt(), 3));
			}
			else
			{
				ui_->tw_methylation->setItem(row_idx, col_idx, GUIHelper::createTableItem(line.at(line_col_idx).trimmed()));
			}

			//apply colors
			//filter column:
			if (header.at(line_col_idx).toLower() == "filter")
			{
				if (line.at(line_col_idx).contains("MultiplePhasingBlocks")) ui_->tw_methylation->item(row_idx, col_idx)->setBackground(QBrush(red_));
				else if (!line.at(line_col_idx).isEmpty()) ui_->tw_methylation->item(row_idx, col_idx)->setBackground(QBrush(orange_));
			}
			//filter z scores
			if (header.at(line_col_idx).startsWith("zscore"))
			{
				if ((line.at(line_col_idx).toDouble() > 3.0) || (line.at(line_col_idx).toDouble() < -3.0))
				{
					ui_->tw_methylation->item(row_idx, col_idx)->setBackground(QBrush(yellow_));
				}
			}


			col_idx++;
		}

		row_idx++;
	}
	//set actual row size
	ui_->tw_methylation->setRowCount(row_idx);

	GUIHelper::resizeTableCellWidths(ui_->tw_methylation);
	ui_->tw_methylation->setAlternatingRowColors(true);
}
