#include "ExpressionExonWidget.h"
#include "RepeatExpansionWidget.h"
#include "ui_ExpressionExonWidget.h"

#include <GUIHelper.h>
#include <Helper.h>
#include <QTime>
#include <TsvFile.h>
#include <VersatileFile.h>

ExpressionExonWidget::ExpressionExonWidget(QString tsv_filename, const QString& variant_gene_filter, const GeneSet& variant_gene_set, QWidget* parent) :
	QWidget(parent),
	tsv_filename_(tsv_filename),
	variant_gene_set_(variant_gene_set),
	ui_(new Ui::ExpressionExonWidget)
{

//	//set gene filter
//	if(!variant_gene_filter.isEmpty())
//	{
//		ui_->gene_filter->setText(variant_gene_filter);
//	}

//	initBiotypeList();

	loadExpressionFile();

//	applyFilters();

	ui_->setupUi(this);
}


ExpressionExonWidget::~ExpressionExonWidget()
{
	delete ui_;
}

void ExpressionExonWidget::loadExpressionFile()
{
	try
	{
		QApplication::setOverrideCursor(Qt::BusyCursor);
		QTime timer;
		timer.start();
		qDebug() << "load expression file...";

		//load TSV file
		TsvFile expression_data;
		QSharedPointer<VersatileFile> expression_data_file = Helper::openVersatileFileForReading(tsv_filename_, false);

		//parse TSV file
		while (!expression_data_file->atEnd())
		{
			QString line = expression_data_file->readLine().trimmed();
			if (line == "")
			{
				// skip empty lines
				continue;
			}
			//TODO: remove when file is fixed
			else if (line.startsWith("#Total library size "))
			{
				//skip special header line
				continue;
			}
			else if	(line.startsWith("##"))
			{
				expression_data.addComment(line);
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


		column_names_.clear();
		numeric_columns_.clear();
		precision_.clear();

		//disable sorting
		ui_->tw_expression_table->setSortingEnabled(false);

		//default columns
		column_names_ << "gene_exon" << "gene_id" << "exon" << "raw" << "rpb" << "srpb" << "gene_name" << "gene_biotype";
		numeric_columns_  << false << false << false << true << true << true << false << false;
		precision_ << -1 << -1 << -1 << 0 << 2 << 2 << -1 << -1;

		//determine col indices for table columns in tsv file
		QVector<int> column_indices;
		foreach (const QString& col_name, column_names_)
		{
			column_indices << expression_data.columnIndex(col_name);
		}


		//create header
		ui_->tw_expression_table->setColumnCount(column_names_.size());
		for (int col_idx = 0; col_idx < column_names_.size(); ++col_idx)
		{
			ui_->tw_expression_table->setHorizontalHeaderItem(col_idx, new QTableWidgetItem(column_names_.at(col_idx)));
		}

		//fill table widget with expression data
		ui_->tw_expression_table->setRowCount(expression_data.rowCount());
		for(int row_idx=0; row_idx<expression_data.rowCount(); ++row_idx)
		{
			QStringList row = expression_data.row(row_idx);

			for (int col_idx = 0; col_idx < column_names_.size(); ++col_idx)
			{

				//get value from file
				if(numeric_columns_.at(col_idx))
				{
					// add numeric QTableWidgetItem
					QString value = row.at(column_indices.at(col_idx));
					if (value != "n/a" && !value.isEmpty())
					{
						QString rounded_number = QString::number(Helper::toDouble(value,
																				  "TSV column " + QString::number(col_idx),
																				  QString::number(row_idx)), 'f', precision_.at(col_idx));
						ui_->tw_expression_table->setItem(row_idx, col_idx, new NumericWidgetItem(rounded_number));
					}
					else
					{
						ui_->tw_expression_table->setItem(row_idx, col_idx, new NumericWidgetItem(""));
					}
				}
				else
				{
					// add standard QTableWidgetItem
					ui_->tw_expression_table->setItem(row_idx, col_idx, new QTableWidgetItem(row.at(column_indices.at(col_idx))));
				}

				//extract gene biotype
				if (column_names_.at(col_idx) == "gene_biotype")
				{
//						QString biotype = row.at(column_indices.at(col_idx));
//						biotypes.insert(biotype.replace('_', ' '));

					//replace '_'
					ui_->tw_expression_table->item(row_idx, col_idx)->setText(ui_->tw_expression_table->item(row_idx, col_idx)->text().replace('_', ' '));
				}
			}
		}

		//hide vertical header
		ui_->tw_expression_table->verticalHeader()->setVisible(false);

		//enable sorting
		ui_->tw_expression_table->setSortingEnabled(true);

		//sort by zscore on default
		ui_->tw_expression_table->sortByColumn(7, Qt::DescendingOrder);

		//optimize table view
		GUIHelper::resizeTableCells(ui_->tw_expression_table, 200, true, 1000);

		//Set number of filtered / total rows
//		ui_->filtered_rows->setText(QByteArray::number(expression_data.rowCount()) + " / " + QByteArray::number(expression_data.rowCount()));

//		//Set cohort size
//		ui_->l_cohort_size->setText("Cohort size: \t " + QString::number(cohort_.size()));


		qDebug() << QString() + "... done(" + Helper::elapsedTime(timer) + ")";

//		qDebug() << "Biotypes:";
//		qDebug() << "file: " << biotypes;
//		qDebug() << "NGSD: " << NGSD().getEnum("gene_transcript", "biotype").toSet();
//		qDebug() << "Diff: " << biotypes.subtract(NGSD().getEnum("gene_transcript", "biotype").toSet());




		QApplication::restoreOverrideCursor();
	}
	catch (Exception& e)
	{
		GUIHelper::showException(this, e, "Error opening RNA expression file.");
	}
}
