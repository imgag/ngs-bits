#include "ExpressionDataWidget.h"
#include "ui_ExpressionDataWidget.h"
#include "TsvFile.h"
#include "Helper.h"
#include "GUIHelper.h"
#include "FilterCascade.h"
#include <QFile>
#include <QMessageBox>

bool NumericWidgetItem::operator<(const QTableWidgetItem& other) const
{
	//convert text to double
	double this_value = Helper::toDouble(this->text());
	double other_value = Helper::toDouble(other.text());
	return (this_value < other_value);
}

ExpressionDataWidget::ExpressionDataWidget(QString tsv_filename, QWidget *parent) :
	QWidget(parent),
	tsv_filename_(tsv_filename),
	ui_(new Ui::ExpressionDataWidget)

{
	ui_->setupUi(this);

	//connect signals and slots
	connect(ui_->apply_filters, SIGNAL(clicked(bool)), this, SLOT(applyFilters()));

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

	//hide rows not passing filters
	for(int row=0; row<row_count; ++row)
	{
		ui_->expressionData->setRowHidden(row, !filter_result.flags()[row]);
	}

	//Set number of filtered / total rows
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

	column_names_ << "raw" << "cpm" << "tpm" << "gene_name";
	numeric_columns_  << true << true << true << false;
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
				ui_->expressionData->setItem(row_idx, col_idx, new NumericWidgetItem(row.at(column_indices.at(col_idx))));
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




