#include "ExpressionExonWidget.h"
#include "RepeatExpansionWidget.h"
#include "ui_ExpressionExonWidget.h"

#include "GUIHelper.h"
#include "Helper.h"
#include "NGSD.h"
#include "VersatileFile.h"
#include "LoginManager.h"
#include "BedFile.h"
#include "GlobalServiceProvider.h"

#include <QChartView>
#include <QMenu>
#include <QMessageBox>
#include <QTime>
QT_CHARTS_USE_NAMESPACE

ExpressionExonWidget::ExpressionExonWidget(QString tsv_filename, int sys_id, QString tissue, const QString& variant_gene_filter, const GeneSet& variant_gene_set, const QString& project,
										   const QString& ps_id, RnaCohortDeterminationStategy cohort_type, QWidget* parent):
	QWidget(parent),
	ui_(new Ui::ExpressionExonWidget),
	tsv_filename_(tsv_filename),
	sys_id_(sys_id),
	tissue_(tissue),
	variant_gene_filter_(variant_gene_filter),
	variant_gene_set_(variant_gene_set),
	project_(project),
	ps_id_(ps_id),
	cohort_type_(cohort_type)
{
	//debug:
	qDebug() << "filename: " << tsv_filename_;

	// skip if no NGSD is available
	if (!LoginManager::active())
	{
		QMessageBox::warning(this, "Expression data widget", "Widget requires NGSD access!");
		return;
	}
	ui_->setupUi(this);

	//connect signals and slots
	connect(ui_->b_apply_filters, SIGNAL(clicked(bool)), this, SLOT(applyFilters()));
	connect(ui_->btn_copy_table,SIGNAL(clicked()),this,SLOT(copyToClipboard()));
	ui_->sa_biotype->setContextMenuPolicy(Qt::CustomContextMenu);
	connect(ui_->sa_biotype,SIGNAL(customContextMenuRequested(QPoint)),this,SLOT(showBiotypeContextMenu(QPoint)));
//	connect(ui_->le_gene_filter, SIGNAL(editingFinished()), this, SLOT(applyFilters()));
//	connect(ui_->sb_min_srpb_sample, SIGNAL(editingFinished()), this, SLOT(applyFilters()));
//	connect(ui_->sb_min_rpb, SIGNAL(editingFinished()), this, SLOT(applyFilters()));
	ui_->tw_expression_table->setContextMenuPolicy(Qt::CustomContextMenu);
	connect(ui_->tw_expression_table,SIGNAL(customContextMenuRequested(QPoint)),this,SLOT(showExpressionTableContextMenu(QPoint)));
	connect(ui_->tw_expression_table,SIGNAL(itemDoubleClicked(QTableWidgetItem*)),this,SLOT(OpenInIGV(QTableWidgetItem*)));

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
		QSet<QByteArray> imported_lines;
		while (!expression_data_file->atEnd())
		{
			QString line = expression_data_file->readLine().replace("\n", "").replace("\r", "");
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
				//skip duplicate lines
				if(imported_lines.contains(line.toUtf8())) continue;

				expression_data_.addRow(line.split('\t'));

				imported_lines.insert(line.toUtf8());
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
	qDebug() << "Init filter";

	//set default values for filter
	ui_->sb_low_expression->setValue(0.1);
	ui_->sb_min_zscore->setValue(2.0);

	initBiotypeList();

	qDebug() << "filter initialized";
}

void ExpressionExonWidget::initTable()
{
	try
	{
		QApplication::setOverrideCursor(Qt::BusyCursor);
		ui_->tw_expression_table->setEnabled(false);
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
		column_names_ << "gene_id" << "exon" << "raw" << "rpb" << "srpb" << "gene_name" << "gene_biotype" << "cohort_mean" << "log2fc" << "zscore" << "pval";
		numeric_columns_ << false << false << true << true << true << false << false << true << true << true << true;
		precision_ << -1 << -1 << 0 << 2 << 2 << -1 << -1 << 2 << 2 << 3 << 3;


		//create header
		ui_->tw_expression_table->setColumnCount(column_names_.size());
		for (int col_idx = 0; col_idx < column_names_.size(); ++col_idx)
		{
			ui_->tw_expression_table->setHorizontalHeaderItem(col_idx, new QTableWidgetItem(column_names_.at(col_idx)));
		}

		qDebug() << "header created";

		qDebug() << QString() + "... done(" + Helper::elapsedTime(timer) + ")";

		ui_->tw_expression_table->setEnabled(true);
		QApplication::restoreOverrideCursor();
	}
	catch (Exception& e)
	{
		ui_->tw_expression_table->setEnabled(true);
		GUIHelper::showException(this, e, "Error creating expression table!");
	}
}

void ExpressionExonWidget::applyFilters()
{
	try
	{
		QApplication::setOverrideCursor(Qt::BusyCursor);
		ui_->tw_expression_table->setEnabled(false);

		filter_result_.reset(true);
		int filtered_lines = expression_data_.rowCount();

		QTime timer;
		timer.start();


		// Filter by parameters stored in file

		// get column index of 'GENES' column
		int gene_idx = expression_data_.headers().indexOf("gene_name");

		if (gene_idx < 0)
		{
			QMessageBox::warning(this, "Filtering error", "Table does not contain a 'gene_name' column! \nFiltering based on gene names is not possible. Please reannotate the RNA sample.");
		}
		else
		{
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
				qDebug() << "\t removed: " << (filtered_lines - filter_result_.countPassing()) << Helper::elapsedTime(timer);
				filtered_lines = filter_result_.countPassing();
			}

			//filter by genes
			GeneSet gene_whitelist = GeneSet::createFromText(ui_->le_gene_filter->text().toUtf8(), ',');
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
				qDebug() << "\t removed: " << (filtered_lines - filter_result_.countPassing()) << Helper::elapsedTime(timer);
				filtered_lines = filter_result_.countPassing();
			}
		}



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
			qDebug() << "\t removed: " << (filtered_lines - filter_result_.countPassing()) << Helper::elapsedTime(timer);
			filtered_lines = filter_result_.countPassing();
		}

		//filter by srpb value
		if (ui_->sb_min_srpb_sample->value() != 0.0)
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
					double min_srpb_value = ui_->sb_min_srpb_sample->value();
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
			qDebug() << "\t removed: " << (filtered_lines - filter_result_.countPassing()) << Helper::elapsedTime(timer);
			filtered_lines = filter_result_.countPassing();
		}

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

		//filter data
		int idx_biotype = expression_data_.headers().indexOf("gene_biotype");
		if (idx_biotype < 0)
		{
			QMessageBox::warning(this, "Filtering error", "Table does not contain a 'gene_biotype' column! \nFiltering based on biotype is not possible. Please reannotate the RNA sample.");
		}
		else
		{
			for(int row_idx=0; row_idx<expression_data_.rowCount(); ++row_idx)
			{
				//skip already filtered
				if (!filter_result_.flags()[row_idx]) continue;

				QString biotype = expression_data_.row(row_idx).at(idx_biotype);
				filter_result_.flags()[row_idx] = selected_biotypes.contains(biotype.replace("_", " "));
			}
		}


		//debug:
		qDebug() << "\t removed (file based): " << (filtered_lines - filter_result_.countPassing()) << Helper::elapsedTime(timer);
		filtered_lines = filter_result_.countPassing();

		//filter by statistical data

		//filter by low expression srpb value
		if (ui_->sb_low_expression->value() != 0)
		{
			qDebug() << "filter by low expression";
			int idx_cohort_mean = expression_data_.headers().indexOf("cohort_mean");
			int idx_srpb = expression_data_.headers().indexOf("srpb");


			if ((idx_cohort_mean == -1))
			{
				QMessageBox::warning(this, "Filtering error", "Table does not contain a 'cohort_mean' column! \nFiltering based on cohort mean value is not possible. Please reannotate the RNA sample.");
			}
			else
			{
				if ((idx_srpb == -1))
				{
					QMessageBox::warning(this, "Filtering error", "Table does not contain a 'srpb' column! \nFiltering based on srpb value is not possible. Please reannotate the RNA sample.");
				}
				else
				{
					try
					{
						double min_expr_value = ui_->sb_low_expression->value();

						for(int row_idx=0; row_idx<expression_data_.rowCount(); ++row_idx)
						{
							//skip already filtered
							if (!filter_result_.flags()[row_idx]) continue;

							QString value_sample = expression_data_.row(row_idx).at(idx_srpb).toUtf8();
							QString value_mean_cohort = expression_data_.row(row_idx).at(idx_cohort_mean).toUtf8();
							if (value_sample.isEmpty() || value_sample == "n/a" || value_mean_cohort.isEmpty() || value_mean_cohort == "n/a")
							{
								filter_result_.flags()[row_idx] = false;
							}
							else
							{
								double tpm_sample = Helper::toDouble(value_sample);
								double tpm_cohort = Helper::toDouble(value_mean_cohort);
								filter_result_.flags()[row_idx] = (tpm_sample >= min_expr_value) || (tpm_cohort >= min_expr_value);
							}
						}
					}
					catch (Exception e)
					{
						QMessageBox::warning(this, "Invalid srpb/cohort mean value", "Couldn't convert given srpb/cohort mean value to number!\n" + e.message());
					}
				}
			}
		}



		//filter by cohort mean
		if (ui_->sb_min_srpb_cohort->value() != 0.0)
		{
			qDebug() << "filter by cohort mean";
			int idx = expression_data_.headers().indexOf("cohort_mean");

			if (idx == -1)
			{
				QMessageBox::warning(this, "Filtering error", "Table does not contain a 'cohort_mean' column! \nFiltering based on cohort mean value is not possible.");
			}
			else
			{
				try
				{
					double min_cohort_mean = ui_->sb_min_srpb_cohort->value();
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
							double cohort_mean = Helper::toDouble(value);
							filter_result_.flags()[row_idx] = cohort_mean >= min_cohort_mean;
						}
					}
				}
				catch (Exception e)
				{
					QMessageBox::warning(this, "Invalid cohort mean value", "Couldn't convert given cohort mean value to number!\n" + e.message());
				}
			}

			//debug:
			qDebug() << "\t removed: " << (filtered_lines - filter_result_.countPassing()) << Helper::elapsedTime(timer);
			filtered_lines = filter_result_.countPassing();
		}

		//filter by log2fc
		if (ui_->sb_min_logfc->value() != 0.0)
		{
			qDebug() << "filter by log fc";
			int idx = expression_data_.headers().indexOf("log2fc");

			if (idx == -1)
			{
				QMessageBox::warning(this, "Filtering error", "Table does not contain a 'log2fc' column! \nFiltering based on log2fc value is not possible.");
			}
			else
			{
				try
				{
					double min_logfc = ui_->sb_min_logfc->value();
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
							double log2fc = Helper::toDouble(value);
							filter_result_.flags()[row_idx] = fabs(log2fc) >= min_logfc;
						}
					}
				}
				catch (Exception e)
				{
					QMessageBox::warning(this, "Invalid log2fc value", "Couldn't convert given log2fc value to number!\n" + e.message());
				}
			}

			//debug:
			qDebug() << "\t removed: " << (filtered_lines - filter_result_.countPassing()) << Helper::elapsedTime(timer);
			filtered_lines = filter_result_.countPassing();
		}

		//filter by zscore
		if (ui_->sb_min_zscore->value() != 0.0)
		{
			qDebug() << "filter by zscore";
			int idx = expression_data_.headers().indexOf("zscore");

			if (idx == -1)
			{
				QMessageBox::warning(this, "Filtering error", "Table does not contain a 'zscore' column! \nFiltering based on zscore value is not possible.");
			}
			else
			{
				try
				{
					double min_zscore = ui_->sb_min_zscore->value();
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
							double zscore = Helper::toDouble(value);
							filter_result_.flags()[row_idx] = fabs(zscore) >= min_zscore;
						}
					}
				}
				catch (Exception e)
				{
					QMessageBox::warning(this, "Invalid zscore value", "Couldn't convert given zscore value to number!\n" + e.message());
				}
			}

			//debug:
			qDebug() << "\t removed: " << (filtered_lines - filter_result_.countPassing()) << Helper::elapsedTime(timer);
			filtered_lines = filter_result_.countPassing();
		}



		//update expression table
		updateTable();


		ui_->tw_expression_table->setEnabled(true);
		QApplication::restoreOverrideCursor();
	}
	catch (Exception& e)
	{
		ui_->tw_expression_table->setEnabled(true);
		GUIHelper::showException(this, e, "Error filtering exon expression file.");
	}
}

void ExpressionExonWidget::copyToClipboard()
{
	GUIHelper::copyToClipboard(ui_->tw_expression_table);
}

void ExpressionExonWidget::showBiotypeContextMenu(QPoint pos)
{
	// create menu
	QMenu menu(ui_->sa_biotype);
	QAction* a_select_all = menu.addAction("Select all biotypes");
	QAction* a_deselect_all = menu.addAction("Deselect all biotypes");
	// execute menu
	QAction* action = menu.exec(ui_->sa_biotype->viewport()->mapToGlobal(pos));
	if (action == nullptr) return;
	// react
	if (action == a_select_all)
	{
		selectAllBiotypes(false);
	}
	else if (action == a_deselect_all)
	{
		selectAllBiotypes(true);
	}
	else
	{
		THROW(ProgrammingException, "Invalid menu action in context menu selected!")
	}
}

void ExpressionExonWidget::selectAllBiotypes(bool deselect)
{
	//set checked state
	foreach (QCheckBox* cb_biotype, ui_->sawc_biotype->findChildren<QCheckBox*>())
	{
		cb_biotype->setChecked(!deselect);
	}
}

void ExpressionExonWidget::showHistogram(int row_idx)
{
	NGSD db;

	BedLine exon = BedLine::fromString(expression_data_.row(row_idx).at(expression_data_.columnIndex("exon")));
	QSet<int> cohort = db.getRNACohort(sys_id_, tissue_, project_, ps_id_, cohort_type_, "exons");
	QVector<double> expr_values = db.getExonExpressionValues(exon, cohort, false);

	if(expr_values.size() == 0) return;
	//create histogram
	std::sort(expr_values.begin(), expr_values.end());
	double median = BasicStatistics::median(expr_values,false);
	double max = ceil(median*2+0.0001);
	Histogram hist(0.0, max, max/40);
	foreach(double expr_value, expr_values)
	{
		hist.inc(expr_value, true);
	}

	//show chart
	QChartView* view = GUIHelper::histogramChart(hist, "Exon expression value distribution (SRPB, " + QString::number(expr_values.size()) + " samples)");
	auto dlg = GUIHelper::createDialog(view, "Exon expression value distribution (" + exon.toString(true) + ")");
	dlg->exec();
}

void ExpressionExonWidget::showExpressionTableContextMenu(QPoint pos)
{
	// create menu
	int row_idx = ui_->tw_expression_table->itemAt(pos)->row();
	QMenu menu(ui_->tw_expression_table);
	QAction* a_show_histogram = menu.addAction("Show histogram");
	QString tpm_mean = ui_->tw_expression_table->item(row_idx, column_names_.indexOf("cohort_mean"))->text();
	if(tpm_mean=="") a_show_histogram->setEnabled(false);

	// execute menu
	QAction* action = menu.exec(ui_->tw_expression_table->viewport()->mapToGlobal(pos));
	if (action == nullptr) return;
	// react
	if (action == a_show_histogram)
	{
		showHistogram(row_idx);
	}
	else
	{
		THROW(ProgrammingException, "Invalid menu action in context menu selected!")
	}
}

void ExpressionExonWidget::OpenInIGV(QTableWidgetItem* item)
{
	qDebug() << "ExpressionDoubleClicked";
	if (item==nullptr) return;

	int gene_col_idx = column_names_.indexOf("exon");
	int row_idx = item->row();

	BedLine exon = BedLine::fromString(ui_->tw_expression_table->item(row_idx, gene_col_idx)->text());

	GlobalServiceProvider::gotoInIGV(exon.toString(true), true);
}


void ExpressionExonWidget::updateTable()
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
		ui_->tw_expression_table->setRowCount(filter_result_.countPassing());
		qDebug() << "row count: " << ui_->tw_expression_table->rowCount();


		//determine col indices for table columns in tsv file
		QVector<int> column_indices;
		foreach (const QString& col_name, column_names_)
		{
			//TODO: Remove when all old RNA files are updated
			//workaround for inconsistant pvalue col name
			if( col_name == "pval")
			{
				int i_pval = -1;
				try
				{
					i_pval = expression_data_.columnIndex(col_name);
				}
				catch (ProgrammingException e)
				{
					i_pval = expression_data_.columnIndex("pvalue");
				}
				column_indices << i_pval;
			}
			else
			{
				column_indices << expression_data_.columnIndex(col_name);
			}
		}
		qDebug() << "header indices parsed";


		// fill table
		int table_row_idx = 0;
		for(int file_line_idx=0; file_line_idx<expression_data_.rowCount(); ++file_line_idx)
		{
			if (!filter_result_.passing(file_line_idx)) continue;


			QStringList row = expression_data_.row(file_line_idx);

			//iterate over columns
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
																				  QString::number(file_line_idx)), 'f', precision_.at(col_idx));
						ui_->tw_expression_table->setItem(table_row_idx, col_idx, new NumericWidgetItem(rounded_number));
					}
					else
					{
						ui_->tw_expression_table->setItem(table_row_idx, col_idx, new NumericWidgetItem(""));
					}
				}
				else
				{
					// add standard QTableWidgetItem
					QTableWidgetItem* cell = new QTableWidgetItem(row.at(column_indices.at(col_idx)));
					cell->setFlags(cell->flags() &  ~Qt::ItemIsEditable);
					ui_->tw_expression_table->setItem(table_row_idx, col_idx, cell);
				}

				//extract gene biotype
				if (column_names_.at(col_idx) == "gene_biotype")
				{
					//replace '_'
					ui_->tw_expression_table->item(table_row_idx, col_idx)->setText(ui_->tw_expression_table->item(table_row_idx, col_idx)->text().replace('_', ' '));
				}

			}

			//update row
			table_row_idx++;
		}




		//enable sorting
		ui_->tw_expression_table->setSortingEnabled(true);

		//sort by zscore on default
		ui_->tw_expression_table->sortByColumn(9, Qt::DescendingOrder);

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

