#include "ExpressionDataWidget.h"
#include "ui_ExpressionDataWidget.h"
#include "TsvFile.h"
#include "Helper.h"
#include "GUIHelper.h"
#include "FilterCascade.h"
#include <QFile>
#include <QMessageBox>
#include "RepeatExpansionWidget.h"

//NumericWidgetItem::NumericWidgetItem(QString text) :
//	QTableWidgetItem(text)
//{
//	this->setTextAlignment(Qt::AlignRight + Qt::AlignVCenter);
//}

//bool NumericWidgetItem::operator<(const QTableWidgetItem& other) const
//{
//	//convert text to double
//	double this_value = Helper::toDouble(this->text());
//	double other_value = Helper::toDouble(other.text());
//	return (this_value < other_value);
//}

ExpressionDataWidget::ExpressionDataWidget(QString tsv_filename, QWidget *parent) :
	QWidget(parent),
	tsv_filename_(tsv_filename),
	ui_(new Ui::ExpressionDataWidget)

{
	ui_->setupUi(this);

	//connect signals and slots
	connect(ui_->apply_filters, SIGNAL(clicked(bool)), this, SLOT(applyFilters()));
	connect(ui_->expressionData->horizontalHeader(), SIGNAL(sectionClicked(int)), this, SLOT(applyFilters()));

	loadExpressionData();
}

ExpressionDataWidget::~ExpressionDataWidget()
{
	delete ui_;
}

void ExpressionDataWidget::applyFilters()
{
	//skip if not necessary
	int row_count = ui_->expressionData->rowCount();
	if (row_count==0) return;

	FilterResult filter_result(row_count);

	//filter by genes
	GeneSet gene_whitelist = GeneSet::createFromText(ui_->gene_filter->text().toLatin1(), ',');
	if (!gene_whitelist.isEmpty())
	{
		QByteArray genes_joined = gene_whitelist.join('|');

		// get column index of 'GENES' column
		int gene_idx = column_names_.indexOf("gene_name");
		if (gene_idx == -1)
		{
			QMessageBox::warning(this, "Filtering error", "Table does not contain a 'gene_name' column! \nFiltering based on genes is not possible.");
		}
		else
		{
			if (genes_joined.contains("*")) //with wildcards
			{
				QRegExp reg(genes_joined.replace("-", "\\-").replace("*", "[A-Z0-9-]*"));
				for(int row_idx=0; row_idx<row_count; ++row_idx)
				{
					if (!filter_result.flags()[row_idx]) continue;

					// generate GeneSet from column text
					GeneSet sv_genes = GeneSet::createFromText(ui_->expressionData->item(row_idx, gene_idx)->text().toLatin1(), ',');

					bool match_found = false;
					foreach(const QByteArray& sv_gene, sv_genes)
					{
						if (reg.exactMatch(sv_gene))
						{
							match_found = true;
							break;
						}
					}
					filter_result.flags()[row_idx] = match_found;
				}
			}
			else //without wildcards
			{
				for(int row_idx=0; row_idx<row_count; ++row_idx)
				{
					if (!filter_result.flags()[row_idx]) continue;

					// generate GeneSet from column text
					GeneSet sv_genes = GeneSet::createFromText(ui_->expressionData->item(row_idx, gene_idx)->text().toLatin1(), ',');

					filter_result.flags()[row_idx] = sv_genes.intersectsWith(gene_whitelist);
				}
			}
		}
	}

	//filter by cohort z-score
	QString zscore = ui_->cohort_zscore->text();
	if (!zscore.isEmpty())
	{
		qDebug() << "z-score filtering";
		int idx = column_names_.indexOf("cohort_zscore");

		double zscore_cutoff = Helper::toDouble(zscore);
		for(int row_idx=0; row_idx<row_count; ++row_idx)
		{
			//skip already filtered
			if (!filter_result.flags()[row_idx]) continue;

			QString value = ui_->expressionData->item(row_idx, idx)->text();
			if (value.isNull() || value.isEmpty() || value == "n/a")
			{
				filter_result.flags()[row_idx] = false;
			}
			else
			{
				double value_dbl = Helper::toDouble(value);
				filter_result.flags()[row_idx] = fabs(value_dbl) >= zscore_cutoff;
			}
		}
	}

	//filter by cohort logFC
	QString logfc = ui_->cohort_logfc->text();
	if (!logfc.isEmpty())
	{
		int idx = column_names_.indexOf("cohort_log2fc");

		double logfc_cutoff = Helper::toDouble(logfc);
		for(int row_idx=0; row_idx<row_count; ++row_idx)
		{
			//skip already filtered
			if (!filter_result.flags()[row_idx]) continue;

			QString value = ui_->expressionData->item(row_idx, idx)->text();
			if (value.isEmpty() || value == "n/a")
			{
				filter_result.flags()[row_idx] = false;
			}
			else
			{
				double value_dbl = Helper::toDouble(value);
				filter_result.flags()[row_idx] = fabs(value_dbl) >= logfc_cutoff;
			}
		}
	}

	//filter by reference logFC
	QString ref_logfc = ui_->ref_logfc->text();
	if (!ref_logfc.isEmpty())
	{
		int idx = column_names_.indexOf("hpa_log2fc");

		double logfc_cutoff = Helper::toDouble(ref_logfc);
		for(int row_idx=0; row_idx<row_count; ++row_idx)
		{
			//skip already filtered
			if (!filter_result.flags()[row_idx]) continue;

			QString value = ui_->expressionData->item(row_idx, idx)->text();
			if (value.isEmpty() || value == "n/a")
			{
				filter_result.flags()[row_idx] = false;
			}
			else
			{
				double value_dbl = Helper::toDouble(value);
				filter_result.flags()[row_idx] = fabs(value_dbl) >= logfc_cutoff;
			}
		}
	}

	//hide rows not passing filters
	for(int row=0; row<row_count; ++row)
	{
		ui_->expressionData->setRowHidden(row, !filter_result.flags()[row]);
	}

	//set number of filtered / total rows
	ui_->filtered_rows->setText(QByteArray::number(filter_result.flags().count(true)) + " / " + QByteArray::number(row_count));

}

void ExpressionDataWidget::loadExpressionData()
{
	//load TSV file
	TsvFile expression_data;
	QSharedPointer<QFile> expression_data_file = Helper::openFileForReading(tsv_filename_, false);

	while (!expression_data_file->atEnd())
	{
		QString line = expression_data_file->readLine().trimmed();
		if (line == "")
		{
			// skip empty lines
			continue;
		}
		else if	(line.startsWith("##"))
		{
			expression_data.addComment(line.mid(2));
		}
		else if (line.startsWith("#"))
		{
			foreach (const QString& header, line.mid(1).split('\t'))
			{
				expression_data.addHeader(header.trimmed());
			}
		}
		else
		{
			expression_data.addRow(line.split('\t'));
		}
	}

	//create table

	//define columns

	column_names_ << "gene_id" << "gene_name" << "raw" << "tpm" << "cohort_log2fc" << "cohort_zscore" << "hpa_log2fc";
	numeric_columns_  << false << false << true << true << true << true << true;
	//determine col indices for table columns in tsv file
	QVector<int> column_indices;
	QStringList tsv_header = expression_data.headers();
	foreach (const QString& col_name, column_names_)
	{
		int column_index = tsv_header.indexOf(col_name);
		if (column_index < 0)
		{
			THROW(FileParseException, "Column name \"" + col_name + "\" not found in TSV file!");
		}
		column_indices << column_index;
	}

	//create header
	ui_->expressionData->setColumnCount(column_names_.size());
	for (int col_idx = 0; col_idx < column_names_.size(); ++col_idx)
	{
		ui_->expressionData->setHorizontalHeaderItem(col_idx, new QTableWidgetItem(column_names_.at(col_idx)));
	}

	//fill table widget with expression data
	ui_->expressionData->setRowCount(expression_data.rowCount());
	for(int row_idx=0; row_idx<expression_data.rowCount(); ++row_idx)
	{
		QStringList row = expression_data.row(row_idx);
		for (int col_idx = 0; col_idx < column_names_.size(); ++col_idx)
		{
			if(numeric_columns_.at(col_idx))
			{
				// add numeric QTableWidgetItem
				QString value = row.at(column_indices.at(col_idx));
				if (value != "n/a" && !value.isEmpty())
				{
					QString rounded_number = QString::number(Helper::toDouble(value,
																			  "TSV column " + QString::number(col_idx),
																			  QString::number(row_idx)), 'f', 2);
					ui_->expressionData->setItem(row_idx, col_idx, new NumericWidgetItem(rounded_number));
				}
				else
				{
					ui_->expressionData->setItem(row_idx, col_idx, new NumericWidgetItem(""));
				}
			}
			else
			{
				// add standard QTableWidgetItem
				ui_->expressionData->setItem(row_idx, col_idx, new QTableWidgetItem(row.at(column_indices.at(col_idx))));
			}
		}
	}

	//hide vertical header
	ui_->expressionData->verticalHeader()->setVisible(false);

	//enable sorting
	ui_->expressionData->setSortingEnabled(true);

	//Set number of filtered / total rows
	ui_->filtered_rows->setText(QByteArray::number(expression_data.rowCount()) + " / " + QByteArray::number(expression_data.rowCount()));
}




