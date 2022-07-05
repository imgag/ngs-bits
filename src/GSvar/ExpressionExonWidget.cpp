#include "ExpressionExonWidget.h"
#include "RepeatExpansionWidget.h"
#include "ui_ExpressionExonWidget.h"

#include "GUIHelper.h"
#include "Helper.h"
#include "NGSD.h"
#include "VersatileFile.h"
#include "LoginManager.h"

#include <QMessageBox>
#include <QTime>

ExpressionExonWidget::ExpressionExonWidget(QString tsv_filename, const QString& variant_gene_filter, const GeneSet& variant_gene_set, QWidget* parent) :
	QWidget(parent),
	tsv_filename_(tsv_filename),
	variant_gene_filter_(variant_gene_filter),
	variant_gene_set_(variant_gene_set),
	ui_(new Ui::ExpressionExonWidget)
{
	// skip if no NGSD is available
	if (!LoginManager::active())
	{
		QMessageBox::warning(this, "Expression data widget", "Widget requires NGSD access!");
		return;
	}
	ui_->setupUi(this);

	//connect signals and slots
	connect(ui_->b_apply_filters, SIGNAL(clicked(bool)), this, SLOT(applyFilters()));

	loadExpressionFile();
	initFilter();
	initTable();
	applyFilters();

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
		expression_data_ = TsvFile();
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
				//parse special header line
				expression_data_.addComment("#" + line);
			}
			else if	(line.startsWith("##"))
			{
				expression_data_.addComment(line);
			}
			else if (line.startsWith("#"))
			{
				foreach (const QString& header, line.mid(1).split('\t'))
				{
					expression_data_.addHeader(header.trimmed());
				}
			}
			else
			{
				expression_data_.addRow(line.split('\t'));
			}
		}

		qDebug() << "TSV parsed";

		//init filter data
		filter_result_ = FilterResult(expression_data_.rowCount());

		QApplication::restoreOverrideCursor();
	}
	catch (Exception& e)
	{
		GUIHelper::showException(this, e, "Error opening RNA expression file.");
	}
}

void ExpressionExonWidget::initFilter()
{
	//TODO: set default values for filter

	initBiotypeList();
}

void ExpressionExonWidget::initTable()
{
	try
	{
		QApplication::setOverrideCursor(Qt::BusyCursor);
		QTime timer;
		timer.start();
		qDebug() << "init expression table...";



		column_names_.clear();
		numeric_columns_.clear();
		precision_.clear();

		qDebug() << "columns cleared";

		//disable sorting
		ui_->tw_expression_table->setSortingEnabled(false);
		qDebug() << "sorting disabled";

		//default columns
		column_names_ << "gene_id" << "exon" << "raw" << "rpb" << "srpb" << "gene_name" << "gene_biotype";
		numeric_columns_ << false << false << true << true << true << false << false;
		precision_ << -1 << -1 << 0 << 2 << 2 << -1 << -1;

		qDebug() << "solumns set";


		//create header
		ui_->tw_expression_table->setColumnCount(column_names_.size());
		for (int col_idx = 0; col_idx < column_names_.size(); ++col_idx)
		{
			ui_->tw_expression_table->setHorizontalHeaderItem(col_idx, new QTableWidgetItem(column_names_.at(col_idx)));
		}

		qDebug() << "header created";



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
		GUIHelper::showException(this, e, "Error creating expression table!");
	}
}

void ExpressionExonWidget::applyFilters()
{
	try
	{
		QApplication::setOverrideCursor(Qt::BusyCursor);

		filter_result_.reset(true);
		int filtered_lines = expression_data_.rowCount();

		// get column index of 'GENES' column
		int gene_idx = -1;
		gene_idx = expression_data_.headers().indexOf("gene_name");

		//filter by variant list gene filter
		if (!variant_gene_set_.isEmpty() && (ui_->cb_filter_by_var_list->checkState() == Qt::Checked))
		{
			qDebug() << "filter by variant gene filter";

			if (gene_idx != -1)
			{
				for(int row_idx=0; row_idx<expression_data_.rowCount(); ++row_idx)
				{
					if (!filter_result_.flags()[row_idx]) continue;

					filter_result_.flags()[row_idx] = variant_gene_set_.contains(expression_data_.row(row_idx).at(gene_idx).toUtf8().trimmed());
				}
			}

			//debug:
			qDebug() << "\t removed: " << (filtered_lines - filter_result_.countPassing());
			filtered_lines = filter_result_.countPassing();
		}

		//filter by genes
		GeneSet gene_whitelist = GeneSet::createFromText(ui_->le_gene_filter->text().toLatin1(), ',');
		if (!gene_whitelist.isEmpty())
		{
			qDebug() << "filter by gene filter";
			QByteArray genes_joined = gene_whitelist.join('|');

			// get column index of 'GENES' column
			if (gene_idx != -1)
			{
				if (genes_joined.contains("*")) //with wildcards
				{
					QRegExp reg(genes_joined.replace("-", "\\-").replace("*", "[A-Z0-9-]*"));
					for(int row_idx=0; row_idx<expression_data_.rowCount(); ++row_idx)
					{
						if (!filter_result_.flags()[row_idx]) continue;

						// generate GeneSet from column text
						GeneSet sv_genes = GeneSet::createFromText(expression_data_.row(row_idx).at(gene_idx).toUtf8(), ',');

						bool match_found = false;
						foreach(const QByteArray& sv_gene, sv_genes)
						{
							if (reg.exactMatch(sv_gene))
							{
								match_found = true;
								break;
							}
						}
						filter_result_.flags()[row_idx] = match_found;
					}
				}
				else //without wildcards
				{
					for(int row_idx=0; row_idx<expression_data_.rowCount(); ++row_idx)
					{
						if (!filter_result_.flags()[row_idx]) continue;

						// generate GeneSet from column text
						GeneSet sv_genes = GeneSet::createFromText(expression_data_.row(row_idx).at(gene_idx).toUtf8(), ',');

						filter_result_.flags()[row_idx] = sv_genes.intersectsWith(gene_whitelist);
					}
				}
			}

			//debug:
			qDebug() << "\t removed: " << (filtered_lines - filter_result_.countPassing());
			filtered_lines = filter_result_.countPassing();
		}


//		//filter by log fold change
//		if (!ui_->abs_logfc->text().isEmpty())
//		{
//			qDebug() << "filter by log fc";
//			int idx = column_names_.indexOf("log2fc");

//			if (idx == -1)
//			{
//				QMessageBox::warning(this, "Filtering error", "Table does not contain a 'log2fc' column! \nFiltering based on fold change is not possible.");
//			}
//			else
//			{
//				try
//				{
//					double logfc_cutoff = ui_->abs_logfc->value();
//					for(int row_idx=0; row_idx<row_count; ++row_idx)
//					{
//						//skip already filtered
//						if (!filter_result.flags()[row_idx]) continue;

//						QString value = ui_->expression_data->item(row_idx, idx)->text();
//						if (value.isEmpty() || value == "n/a")
//						{
//							filter_result.flags()[row_idx] = false;
//						}
//						else
//						{
//							double value_dbl = Helper::toDouble(value);
//							filter_result.flags()[row_idx] = fabs(value_dbl) >= logfc_cutoff;
//						}
//					}
//				}
//				catch (Exception e)
//				{
//					QMessageBox::warning(this, "Invalid log fold change value", "Couldn't convert given fold change value to number!\n" + e.message());
//					return;
//				}
//			}
//		}

//		//filter by cohort z-score
//		if (!ui_->abs_zscore->text().isEmpty())
//		{
//			int idx = column_names_.indexOf("zscore");

//			if (idx == -1)
//			{
//				QMessageBox::warning(this, "Filtering error", "Table does not contain a 'zscore' column! \nFiltering based on z-score is not possible.");
//			}
//			else
//			{
//				try
//				{
//					double zscore_cutoff = ui_->abs_zscore->value();
//					for(int row_idx=0; row_idx<row_count; ++row_idx)
//					{
//						//skip already filtered
//						if (!filter_result.flags()[row_idx]) continue;

//						QString value = ui_->expression_data->item(row_idx, idx)->text();
//						if (value.isNull() || value.isEmpty() || value == "n/a")
//						{
//							filter_result.flags()[row_idx] = false;
//						}
//						else
//						{
//							double value_dbl = Helper::toDouble(value);
//							filter_result.flags()[row_idx] = fabs(value_dbl) >= zscore_cutoff;
//						}
//					}
//				}
//				catch (Exception e)
//				{
//					QMessageBox::warning(this, "Invalid zscore value", "Couldn't convert given zscore value to number!\n" + e.message());
//					return;
//				}
//			}
//		}

		//filter by rpb value
		if (!ui_->sb_min_rpb->text().isEmpty())
		{
			qDebug() << "filter by rpb";
			int idx = expression_data_.headers().indexOf("rpb");

			if (idx == -1)
			{
				QMessageBox::warning(this, "Filtering error", "Table does not contain a 'rpb' column! \nFiltering based on rpb value is not possible.");
			}
			else
			{
				try
				{
					double min_rpb_value = ui_->sb_min_rpb->value();
					for(int row_idx=0; row_idx<expression_data_.rowCount(); ++row_idx)
					{
						//skip already filtered
						if (!filter_result_.flags()[row_idx]) continue;

						QString value = expression_data_.row(row_idx).at(idx).toUtf8();
						if (value.isEmpty() || value == "n/a")
						{
							filter_result_.flags()[row_idx] = false;
						}
						else
						{
							double tpm_value = Helper::toDouble(value);
							filter_result_.flags()[row_idx] = tpm_value >= min_rpb_value;
						}
					}
				}
				catch (Exception e)
				{
					QMessageBox::warning(this, "Invalid rpb value", "Couldn't convert given rpb value to number!\n" + e.message());
				}
			}

			//debug:
			qDebug() << "\t removed: " << (filtered_lines - filter_result_.countPassing());
			filtered_lines = filter_result_.countPassing();
		}

		//filter by srpb value
		if (!ui_->sb_min_srpb->text().isEmpty())
		{
			qDebug() << "filter by srpb";
			int idx = column_names_.indexOf("srpb");

			if (idx == -1)
			{
				QMessageBox::warning(this, "Filtering error", "Table does not contain a 'srpb' column! \nFiltering based on srpb value is not possible.");
			}
			else
			{
				try
				{
					double min_srpb_value = ui_->sb_min_srpb->value();
					for(int row_idx=0; row_idx<expression_data_.rowCount(); ++row_idx)
					{
						//skip already filtered
						if (!filter_result_.flags()[row_idx]) continue;

						QString value = expression_data_.row(row_idx).at(idx).toUtf8();
						if (value.isEmpty() || value == "n/a")
						{
							filter_result_.flags()[row_idx] = false;
						}
						else
						{
							double srpb_value = Helper::toDouble(value);
							filter_result_.flags()[row_idx] = srpb_value >= min_srpb_value;
						}
					}
				}
				catch (Exception e)
				{
					QMessageBox::warning(this, "Invalid srpb value", "Couldn't convert given srpb value to number!\n" + e.message());
				}
			}

			//debug:
			qDebug() << "\t removed: " << (filtered_lines - filter_result_.countPassing());
			filtered_lines = filter_result_.countPassing();
		}

//		//filter by cohort tpm value
//		if (!ui_->tpm_cohort_value->text().isEmpty())
//		{
//			int idx = column_names_.indexOf("cohort_mean");
//			double min_tpm_value = ui_->tpm_cohort_value->value();

//			for(int row_idx=0; row_idx<row_count; ++row_idx)
//			{
//				//skip already filtered
//				if (!filter_result.flags()[row_idx]) continue;

//				QString value = ui_->expression_data->item(row_idx, idx)->text();
//				if (value.isEmpty() || value == "n/a")
//				{
//					filter_result.flags()[row_idx] = false;
//				}
//				else
//				{
//					double tpm_value = Helper::toDouble(value);
//					filter_result.flags()[row_idx] = tpm_value >= min_tpm_value;
//				}
//			}
//		}

//		//filter by low expression srpb value
//		if (!ui_->sb_low_expression->text().isEmpty())
//		{
//			double low_expr_srpb = ui_->sb_low_expression->value();

//			for(int row_idx=0; row_idx<expression_data_.rowCount(); ++row_idx)
//			{
//				//skip already filtered
//				if (!filter_result_.flags()[row_idx]) continue;

//				QString value_sample = expression_data_.row(row_idx).at(column_names_.indexOf("srpb"));
//				QString value_cohort = expression_data_.row(row_idx).at(column_names_.indexOf("cohort_mean"));
//				if (value_sample.isEmpty() || value_sample == "n/a" || value_cohort.isEmpty() || value_cohort == "n/a")
//				{
//					filter_result_.flags()[row_idx] = false;
//				}
//				else
//				{
//					double srpb_sample = Helper::toDouble(value_sample);
//					double srpb_cohort = Helper::toDouble(value_cohort);
//					filter_result_.flags()[row_idx] = (srpb_sample >= low_expr_srpb) || (srpb_cohort >= low_expr_srpb);
//				}
//			}
//		}


		//filter by biotype
		QSet<QString> selected_biotypes;
		qDebug() << "filter by biotype";

		//get selected biotypes
		foreach (QCheckBox* cb_biotype, ui_->sawc_biotype->findChildren<QCheckBox*>())
		{
			if (cb_biotype->isChecked())
			{
				selected_biotypes.insert(cb_biotype->text());
			}
		}

		qDebug() << selected_biotypes;

		//filter data
		int idx_biotype = expression_data_.headers().indexOf("gene_biotype");

		for(int row_idx=0; row_idx<expression_data_.rowCount(); ++row_idx)
		{
			//skip already filtered
			if (!filter_result_.flags()[row_idx]) continue;

			QString biotype = expression_data_.row(row_idx).at(idx_biotype);
			qDebug() << biotype;
			filter_result_.flags()[row_idx] = selected_biotypes.contains(biotype.replace("_", " "));
		}

		//debug:
		qDebug() << "\t removed: " << (filtered_lines - filter_result_.countPassing());
		filtered_lines = filter_result_.countPassing();



		//update expression table
		updateTable();


		QApplication::restoreOverrideCursor();
	}
	catch (Exception& e)
	{
		GUIHelper::showException(this, e, "Error filtering exon expression file.");
	}
}

void ExpressionExonWidget::updateTable(int max_rows)
{
	try
	{
		//fill table widget with expression data
		QApplication::setOverrideCursor(Qt::BusyCursor);
		QTime timer;
		timer.start();

		//disable sorting
		ui_->tw_expression_table->setSortingEnabled(false);

		qDebug() << "update expression table...";


		//update dimensions
		qDebug() << "passing count: " << filter_result_.countPassing();
		ui_->tw_expression_table->setRowCount(std::min(filter_result_.countPassing(), max_rows));
		qDebug() << "row count: " << ui_->tw_expression_table->rowCount();


		//determine col indices for table columns in tsv file
		QVector<int> column_indices;
		foreach (const QString& col_name, column_names_)
		{
			column_indices << expression_data_.columnIndex(col_name);
		}
		qDebug() << "header indices parsed";

		int row_idx = 0;
		for(int line_idx=0; line_idx<expression_data_.rowCount(); ++line_idx)
		{
			if (!filter_result_.passing(line_idx)) continue;


			QStringList row = expression_data_.row(line_idx);

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
																				  QString::number(line_idx)), 'f', precision_.at(col_idx));
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
					//replace '_'
					ui_->tw_expression_table->item(row_idx, col_idx)->setText(ui_->tw_expression_table->item(row_idx, col_idx)->text().replace('_', ' '));
				}
			}

			if (row_idx % 1000 == 0) qDebug() << "Row: " << row_idx;
			//update row
			row_idx++;
			//abort if max row count is reached
			if (row_idx>=max_rows) break;
		}

		//enable sorting
		ui_->tw_expression_table->setSortingEnabled(true);

		//sort by zscore on default
//		ui_->tw_expression_table->sortByColumn(7, Qt::DescendingOrder);

		//set number of filtered / total rows
		ui_->l_filtered_rows->setText(QByteArray::number(filter_result_.flags().count(true)) + " / " + QByteArray::number(expression_data_.rowCount()));

		//optimize table view
		GUIHelper::resizeTableCells(ui_->tw_expression_table, 350, true, 1000);


		QApplication::restoreOverrideCursor();
	}
	catch (Exception& e)
	{
		GUIHelper::showException(this, e, "Error updating expression table!");
	}
}

void ExpressionExonWidget::initBiotypeList()
{
	qDebug()<< "init biotype list";
	//biotypes
	QStringList sorted_biotypes = NGSD().getEnum("gene_transcript", "biotype");
	std::sort(sorted_biotypes.begin(), sorted_biotypes.end());
	QVBoxLayout* vbox = new QVBoxLayout;
	foreach (const QString& biotype, sorted_biotypes)
	{
		QCheckBox* cb_biotype = new QCheckBox(biotype);
		cb_biotype->setChecked((biotype == "protein coding"));
		vbox->addWidget(cb_biotype);
	}
	ui_->sawc_biotype->setLayout(vbox);
}
