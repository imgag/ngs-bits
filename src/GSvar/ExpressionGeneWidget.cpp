#include "ExpressionGeneWidget.h"
#include "ui_ExpressionGeneWidget.h"
#include "TsvFile.h"
#include "Helper.h"
#include "GUIHelper.h"
#include "FilterCascade.h"
#include <NGSD.h>
#include <QCheckBox>
#include <QFile>
#include <QMenu>
#include <QMessageBox>
#include "RepeatExpansionWidget.h"
#include "LoginManager.h"
#include <QChartView>
#include <QSignalMapper>
QT_CHARTS_USE_NAMESPACE



ExpressionGeneWidget::ExpressionGeneWidget(QString tsv_filename, int sys_id, QString tissue, const QString& variant_gene_filter, const GeneSet& variant_gene_set, const QString& project,
										   const QString& ps_id, RnaCohortDeterminationStategy cohort_type, QWidget *parent) :
	QWidget(parent),
	tsv_filename_(tsv_filename),
	sys_id_(sys_id),
	tissue_(tissue),
	variant_gene_set_(variant_gene_set),
	project_(project),
	ps_id_(ps_id),
	cohort_type_(cohort_type),
	ui_(new Ui::ExpressionGeneWidget)

{
	// skipp if no NGSD is available
	if (!LoginManager::active())
	{
		QMessageBox::warning(this, "Expression data widget", "Widget requires NGSD access!");
		return;
	}
	ui_->setupUi(this);

	//connect signals and slots
	connect(ui_->apply_filters, SIGNAL(clicked(bool)), this, SLOT(applyFilters()));
	connect(ui_->expression_data->horizontalHeader(), SIGNAL(sectionClicked(int)), this, SLOT(applyFilters()));
	connect(ui_->btn_copy_table,SIGNAL(clicked()),this,SLOT(copyToClipboard()));
	connect(ui_->sa_biotype,SIGNAL(customContextMenuRequested(QPoint)),this,SLOT(showBiotypeContextMenu(QPoint)));
	ui_->expression_data->setContextMenuPolicy(Qt::CustomContextMenu);
	connect(ui_->expression_data,SIGNAL(customContextMenuRequested(QPoint)),this,SLOT(showExpressionTableContextMenu(QPoint)));
	connect(ui_->gene_filter, SIGNAL(editingFinished()), this, SLOT(applyFilters()));
	connect(ui_->abs_logfc, SIGNAL(editingFinished()), this, SLOT(applyFilters()));
	connect(ui_->abs_zscore, SIGNAL(editingFinished()), this, SLOT(applyFilters()));
	connect(ui_->tpm_value, SIGNAL(editingFinished()), this, SLOT(applyFilters()));
	connect(ui_->tpm_cohort_value, SIGNAL(editingFinished()), this, SLOT(applyFilters()));
	connect(ui_->low_expr_tpm, SIGNAL(editingFinished()), this, SLOT(applyFilters()));
	connect(ui_->show_cohort, SIGNAL(clicked(bool)), this, SLOT(showCohort()));


	// set context menus for biotype filter
	ui_->sa_biotype->setContextMenuPolicy(Qt::CustomContextMenu);

	// set initial cohort determination strategy
	if (cohort_type == RNA_COHORT_GERMLINE) ui_->rb_germline_tissue->setChecked(true);
	if (cohort_type == RNA_COHORT_GERMLINE_PROJECT) ui_->rb_germline_project->setChecked(true);
	if (cohort_type == RNA_COHORT_SOMATIC) ui_->rb_somatic->setChecked(true);


	//(de-)activate varaint list gene filter
	if (!variant_gene_set_.isEmpty())
	{
		ui_->cb_filter_by_var_list->setEnabled(true);
		ui_->cb_filter_by_var_list->setChecked(true);

		//Add tool tip
		QString tool_tip = QString("<p>") + QString::number(variant_gene_set_.count()) + " genes selected:<br><br>";
		QStringList genes;
		int i = 0;
		foreach (const QString& gene, variant_gene_set_.toStringList())
		{
			genes << gene;
			i++;
			if(i > 100) break;
		}
		if(genes.size() < variant_gene_set_.count()) genes << "...</p>";
		tool_tip += genes.join(", ");
		ui_->cb_filter_by_var_list->setToolTip(tool_tip);
	}

	//set gene filter
	if(!variant_gene_filter.isEmpty())
	{
		ui_->gene_filter->setText(variant_gene_filter);
	}

	initBiotypeList();

	loadExpressionData();

	applyFilters();


}

ExpressionGeneWidget::~ExpressionGeneWidget()
{
	delete ui_;
}

void ExpressionGeneWidget::applyFilters()
{
	//skip if not necessary
	int row_count = ui_->expression_data->rowCount();
	if (row_count==0) return;

	try
	{
		QApplication::setOverrideCursor(Qt::BusyCursor);

		FilterResult filter_result(row_count);

		//update filter strategy
		RnaCohortDeterminationStategy cohort_type;
		if (ui_->rb_germline_tissue->isChecked())
		{
			cohort_type = RNA_COHORT_GERMLINE;
		}
		else if (ui_->rb_germline_project->isChecked())
		{
			cohort_type = RNA_COHORT_GERMLINE_PROJECT;
		}
		else if (ui_->rb_somatic->isChecked())
		{
			cohort_type = RNA_COHORT_SOMATIC;
		}
		else
		{
			THROW(ArgumentException, "Invalid cohort type!");
		}

		if (cohort_type != cohort_type_)
		{
			//update cohort determination strategy
			cohort_type_ = cohort_type;

			//rebuild table
			loadExpressionData();

		}

		// get column index of 'GENES' column
		int gene_idx = -1;
		gene_idx = column_names_.indexOf("gene_name");


		//filter by variant list gene filter
		if (!variant_gene_set_.isEmpty() && (ui_->cb_filter_by_var_list->checkState() == Qt::Checked))
		{
			qDebug() << "filter by variant gene filter";

			if (gene_idx != -1)
			{
				for(int row_idx=0; row_idx<row_count; ++row_idx)
				{
					if (!filter_result.flags()[row_idx]) continue;

					filter_result.flags()[row_idx] = variant_gene_set_.contains(ui_->expression_data->item(row_idx, gene_idx)->text().toUtf8().trimmed());
				}
			}
		}

		//filter by genes
		GeneSet gene_whitelist = GeneSet::createFromText(ui_->gene_filter->text().toLatin1(), ',');
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
					for(int row_idx=0; row_idx<row_count; ++row_idx)
					{
						if (!filter_result.flags()[row_idx]) continue;

						// generate GeneSet from column text
						GeneSet sv_genes = GeneSet::createFromText(ui_->expression_data->item(row_idx, gene_idx)->text().toLatin1(), ',');

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
						GeneSet sv_genes = GeneSet::createFromText(ui_->expression_data->item(row_idx, gene_idx)->text().toLatin1(), ',');

						filter_result.flags()[row_idx] = sv_genes.intersectsWith(gene_whitelist);
					}
				}
			}
		}

		//filter by log fold change
		if (!ui_->abs_logfc->text().isEmpty())
		{
			qDebug() << "filter by log fc";
			int idx = column_names_.indexOf("log2fc");

			if (idx == -1)
			{
				QMessageBox::warning(this, "Filtering error", "Table does not contain a 'log2fc' column! \nFiltering based on fold change is not possible.");
			}
			else
			{
				try
				{
					double logfc_cutoff = ui_->abs_logfc->value();
					for(int row_idx=0; row_idx<row_count; ++row_idx)
					{
						//skip already filtered
						if (!filter_result.flags()[row_idx]) continue;

						QString value = ui_->expression_data->item(row_idx, idx)->text();
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
				catch (Exception e)
				{
					QMessageBox::warning(this, "Invalid log fold change value", "Couldn't convert given fold change value to number!\n" + e.message());
					return;
				}
			}
		}

		//filter by cohort z-score
		if (!ui_->abs_zscore->text().isEmpty())
		{
			int idx = column_names_.indexOf("zscore");

			if (idx == -1)
			{
				QMessageBox::warning(this, "Filtering error", "Table does not contain a 'zscore' column! \nFiltering based on z-score is not possible.");
			}
			else
			{
				try
				{
					double zscore_cutoff = ui_->abs_zscore->value();
					for(int row_idx=0; row_idx<row_count; ++row_idx)
					{
						//skip already filtered
						if (!filter_result.flags()[row_idx]) continue;

						QString value = ui_->expression_data->item(row_idx, idx)->text();
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
				catch (Exception e)
				{
					QMessageBox::warning(this, "Invalid zscore value", "Couldn't convert given zscore value to number!\n" + e.message());
					return;
				}
			}
		}

		//filter by tpm value
		if (!ui_->tpm_value->text().isEmpty())
		{
			qDebug() << "filter by tpm";
			int idx = column_names_.indexOf("tpm");

			if (idx == -1)
			{
				QMessageBox::warning(this, "Filtering error", "Table does not contain a 'tpm' column! \nFiltering based on tpm value is not possible.");
			}
			else
			{
				try
				{
					double min_tpm_value = ui_->tpm_value->value();
					for(int row_idx=0; row_idx<row_count; ++row_idx)
					{
						//skip already filtered
						if (!filter_result.flags()[row_idx]) continue;

						QString value = ui_->expression_data->item(row_idx, idx)->text();
						if (value.isEmpty() || value == "n/a")
						{
							filter_result.flags()[row_idx] = false;
						}
						else
						{
							double tpm_value = Helper::toDouble(value);
							filter_result.flags()[row_idx] = tpm_value >= min_tpm_value;
						}
					}
				}
				catch (Exception e)
				{
					QMessageBox::warning(this, "Invalid tpm value", "Couldn't convert given tpm value to number!\n" + e.message());
				}
			}
		}

		//filter by cohort tpm value
		if (!ui_->tpm_cohort_value->text().isEmpty())
		{
			int idx = column_names_.indexOf("cohort_mean");
			double min_tpm_value = ui_->tpm_cohort_value->value();

			for(int row_idx=0; row_idx<row_count; ++row_idx)
			{
				//skip already filtered
				if (!filter_result.flags()[row_idx]) continue;

				QString value = ui_->expression_data->item(row_idx, idx)->text();
				if (value.isEmpty() || value == "n/a")
				{
					filter_result.flags()[row_idx] = false;
				}
				else
				{
					double tpm_value = Helper::toDouble(value);
					filter_result.flags()[row_idx] = tpm_value >= min_tpm_value;
				}
			}
		}

		//filter by low expression tpm value
		if (!ui_->low_expr_tpm->text().isEmpty())
		{
			double low_expr_tpm = ui_->low_expr_tpm->value();

			for(int row_idx=0; row_idx<row_count; ++row_idx)
			{
				//skip already filtered
				if (!filter_result.flags()[row_idx]) continue;

				QString value_sample = ui_->expression_data->item(row_idx, column_names_.indexOf("tpm"))->text();
				QString value_cohort = ui_->expression_data->item(row_idx, column_names_.indexOf("cohort_mean"))->text();
				if (value_sample.isEmpty() || value_sample == "n/a" || value_cohort.isEmpty() || value_cohort == "n/a")
				{
					filter_result.flags()[row_idx] = false;
				}
				else
				{
					double tpm_sample = Helper::toDouble(value_sample);
					double tpm_cohort = Helper::toDouble(value_cohort);
					filter_result.flags()[row_idx] = (tpm_sample >= low_expr_tpm) || (tpm_cohort >= low_expr_tpm);
				}
			}
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
		int idx_biotype = column_names_.indexOf("gene_biotype");

		for(int row_idx=0; row_idx<row_count; ++row_idx)
		{
			//skip already filtered
			if (!filter_result.flags()[row_idx]) continue;

			QString biotype = ui_->expression_data->item(row_idx, idx_biotype)->text();
			filter_result.flags()[row_idx] = selected_biotypes.contains(biotype);
		}


		//hide rows not passing filters
		for(int row=0; row<row_count; ++row)
		{
			ui_->expression_data->setRowHidden(row, !filter_result.flags()[row]);
		}

		//set number of filtered / total rows
		ui_->filtered_rows->setText(QByteArray::number(filter_result.flags().count(true)) + " / " + QByteArray::number(row_count));


		QApplication::restoreOverrideCursor();
	}
	catch (Exception& e)
	{
		GUIHelper::showException(this, e, "Error opening RNA expression file.");
	}


}

void ExpressionGeneWidget::copyToClipboard()
{
	GUIHelper::copyToClipboard(ui_->expression_data);
}

void ExpressionGeneWidget::showBiotypeContextMenu(QPoint pos)
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

void ExpressionGeneWidget::selectAllBiotypes(bool deselect)
{
	//set checked state
	foreach (QCheckBox* cb_biotype, ui_->sawc_biotype->findChildren<QCheckBox*>())
	{
		cb_biotype->setChecked(!deselect);
	}
}

void ExpressionGeneWidget::showHistogram(int row_idx)
{
	NGSD db;
	QByteArray ensg = ui_->expression_data->item(row_idx, 0)->text().toUtf8();
	QVector<double> expr_values = db.getGeneExpressionValues(db.getEnsemblGeneMapping().value(ensg), cohort_, true);

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
	QChartView* view = GUIHelper::histogramChart(hist, "Expression value distribution (log2_TPM, " + QString::number(expr_values.size()) + " samples)");
	auto dlg = GUIHelper::createDialog(view, "Expression value distribution (" + ensg + ")");
	dlg->exec();
}

void ExpressionGeneWidget::showExpressionTableContextMenu(QPoint pos)
{
	// create menu
	int row_idx = ui_->expression_data->itemAt(pos)->row();
	QMenu menu(ui_->expression_data);
	QAction* a_show_histogram = menu.addAction("Show histogram");
	QString tpm_mean = ui_->expression_data->item(row_idx, column_names_.indexOf("cohort_mean"))->text();
	if(tpm_mean=="") a_show_histogram->setEnabled(false);

	// execute menu
	QAction* action = menu.exec(ui_->expression_data->viewport()->mapToGlobal(pos));
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

void ExpressionGeneWidget::showCohort()
{
	if (!LoginManager::active()) return;
	NGSD db;
	try
	{
		QDialog cohort_dialog(this);
		cohort_dialog.setWindowFlags(Qt::Window);
		cohort_dialog.setWindowTitle("Cohort of Sample " + db.processedSampleName(ps_id_));
		cohort_dialog.setLayout(new QBoxLayout(QBoxLayout::TopToBottom));
		cohort_dialog.layout()->setMargin(3);

		//add description:
		QLabel* description = new QLabel("The cohort contains the following samples:");
		cohort_dialog.layout()->addWidget(description);

		//add table
		cohort_table_ = new QTableWidget(cohort_.size(), 1);
		cohort_dialog.layout()->addWidget(cohort_table_);

		//fill table
		cohort_table_->setHorizontalHeaderItem(0, GUIHelper::createTableItem("Processed sample names"));
		QStringList cohort_samples;
		foreach (int i, cohort_)
		{
			cohort_samples << db.processedSampleName(QString::number(i));
		}
		std::sort(cohort_samples.begin(), cohort_samples.end());
		for (int r = 0; r < cohort_samples.size(); ++r)
		{
			cohort_table_->setItem(r, 0, GUIHelper::createTableItem(cohort_samples.at(r)));
		}
		GUIHelper::resizeTableCells(cohort_table_);

		//add copy button
		QWidget* h_box = new QWidget();
		QHBoxLayout* h_layout = new QHBoxLayout();
		QSpacerItem* h_spacer = new QSpacerItem(40, 20, QSizePolicy::Expanding);
		QPushButton* copy_button = new QPushButton(QIcon(":/Icons/CopyClipboard.png"), "");
		connect(copy_button,SIGNAL(clicked()),this, SLOT(copyCohortToClipboard()));

		h_layout->addItem(h_spacer);
		h_layout->addWidget(copy_button);
		h_box->setLayout(h_layout);
		cohort_dialog.layout()->setMargin(0);
		cohort_dialog.layout()->addWidget(h_box);


		//show dialog
		cohort_dialog.exec();
	}
	catch (Exception& e)
	{
		GUIHelper::showException(this, e, "Error opening RNA expression file.");
	}
}

void ExpressionGeneWidget::copyCohortToClipboard()
{
	if (cohort_table_ == nullptr) return;
	GUIHelper::copyToClipboard(cohort_table_);
}

void ExpressionGeneWidget::loadExpressionData()
{
	try
	{
		QApplication::setOverrideCursor(Qt::BusyCursor);
		QTime timer;
		timer.start();
		qDebug() << "load expression file...";

		//skip without database
		if (!LoginManager::active()) return;

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

		//get RNA stats from NGSD
		QMap<QByteArray, ExpressionStats> expression_stats = NGSD().calculateCohortExpressionStatistics(sys_id_, tissue_,  cohort_, project_, ps_id_, cohort_type_);

		column_names_.clear();
		numeric_columns_.clear();
		precision_.clear();

		//disable sorting
		ui_->expression_data->setSortingEnabled(false);

		//default columns
		column_names_ << "gene_id" << "gene_name" << "gene_biotype" << "raw" << "tpm";
		numeric_columns_  << false << false << false << true << true;
		precision_ << -1 << -1 << -1 << 0 << 2;

		//db columns
		QStringList db_column_names;
		bool symbol_in_ngsd = false;
		ExpressionStats gene_stats;
		double log2fc=0, zscore=0, p_value=0;

		db_column_names = QStringList()  << "cohort_mean" << "log2fc" << "zscore" << "pvalue";
		column_names_ << db_column_names;
		numeric_columns_  << true << true << true << true;
		precision_ << 2 << 2 << 3 << 3;

		//add hpa columns if available
		QStringList headers = expression_data.headers();
		if (headers.contains("hpa_tissue_tpm"))
		{
			column_names_ << "hpa_tissue_tpm" << "hpa_tissue_log2tpm" << "hpa_sample_log2tpm" << "hpa_log2fc";
			numeric_columns_  << true << true << true << true;
			precision_ << 3 << 3 << 3 << 3;
		}


		//determine col indices for table columns in tsv file
		QVector<int> column_indices;
		foreach (const QString& col_name, column_names_)
		{
			if(headers.contains(col_name))
			{
				column_indices << expression_data.columnIndex(col_name);
			}
			else
			{
				column_indices << -1;
			}
		}

		//cohort correlation
		QVector<double> cohort_means;
		QVector<double> tpm_values;

		//create header
		ui_->expression_data->setColumnCount(column_names_.size());
		for (int col_idx = 0; col_idx < column_names_.size(); ++col_idx)
		{
			ui_->expression_data->setHorizontalHeaderItem(col_idx, new QTableWidgetItem(column_names_.at(col_idx)));
		}

		//fill table widget with expression data
		ui_->expression_data->setRowCount(expression_data.rowCount());
		for(int row_idx=0; row_idx<expression_data.rowCount(); ++row_idx)
		{
			QStringList row = expression_data.row(row_idx);
			QByteArray symbol = row.at(column_indices.at(1)).toUtf8();
			double tpm = Helper::toDouble(row.at(column_indices.at(column_names_.indexOf("tpm"))), "tpm", QString::number(row_idx));
			double log2p1tpm = std::log2(tpm + 1);

			symbol_in_ngsd = expression_stats.contains(symbol);
			if (symbol_in_ngsd)
			{
				gene_stats = expression_stats.value(symbol);
				log2fc = log2p1tpm - std::log2(gene_stats.mean + 1);
				zscore = (log2p1tpm - gene_stats.mean_log2) / gene_stats.stddev_log2;
				p_value = 1 + std::erf(- std::abs(zscore) / std::sqrt(2));
			}

			for (int col_idx = 0; col_idx < column_names_.size(); ++col_idx)
			{
				if(db_column_names.contains(column_names_.at(col_idx)))
				{
					//get value from NGSD
					if(symbol_in_ngsd)
					{
						if(column_names_.at(col_idx) == "cohort_mean")
						{
							ui_->expression_data->setItem(row_idx, col_idx, new NumericWidgetItem(QString::number(gene_stats.mean, 'f', precision_.at(col_idx))));
							if((gene_stats.mean > 0) && (tpm > 0))
							{
								cohort_means << gene_stats.mean;
								tpm_values << tpm;
							}
						}
						else if(column_names_.at(col_idx) == "log2fc")
						{
							ui_->expression_data->setItem(row_idx, col_idx, new NumericWidgetItem(QString::number(log2fc, 'f', precision_.at(col_idx))));
						}
						else if(column_names_.at(col_idx) == "zscore")
						{
							ui_->expression_data->setItem(row_idx, col_idx, new NumericWidgetItem(QString::number(zscore, 'f', precision_.at(col_idx))));
						}
						else if(column_names_.at(col_idx) == "pvalue")
						{
							ui_->expression_data->setItem(row_idx, col_idx, new NumericWidgetItem(QString::number(p_value, 'f', precision_.at(col_idx))));
						}
						else
						{
							THROW(ArgumentException, "Invalid db column '" + column_names_.at(col_idx) + "'!");
						}
					}
					else
					{
						ui_->expression_data->setItem(row_idx, col_idx, new NumericWidgetItem(""));
					}
				}
				else
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
							ui_->expression_data->setItem(row_idx, col_idx, new NumericWidgetItem(rounded_number));
						}
						else
						{
							ui_->expression_data->setItem(row_idx, col_idx, new NumericWidgetItem(""));
						}
					}
					else
					{
						// add standard QTableWidgetItem
						QTableWidgetItem* cell = new QTableWidgetItem(row.at(column_indices.at(col_idx)));
						cell->setFlags(cell->flags() &  ~Qt::ItemIsEditable);
						ui_->expression_data->setItem(row_idx, col_idx, cell);
					}

					//extract gene biotype
					if ((column_names_.at(col_idx) == "gene_biotype") || (column_names_.at(col_idx) == "biotype"))
					{
						//replace '_'
						ui_->expression_data->item(row_idx, col_idx)->setText(ui_->expression_data->item(row_idx, col_idx)->text().replace('_', ' '));
					}
				}
			}
		}

		//compute cohort correlation
		QVector<double> rank_sample = calculateRanks(tpm_values);
		QVector<double> rank_means = calculateRanks(cohort_means);
		double correlation = BasicStatistics::correlation(rank_sample, rank_means);

		//hide vertical header
		ui_->expression_data->verticalHeader()->setVisible(false);

		//enable sorting
		ui_->expression_data->setSortingEnabled(true);

		//sort by zscore on default
		ui_->expression_data->sortByColumn(7, Qt::DescendingOrder);

		//optimize table view
		GUIHelper::resizeTableCells(ui_->expression_data, 200, true, 1000);

		//Set number of filtered / total rows
		ui_->filtered_rows->setText(QByteArray::number(expression_data.rowCount()) + " / " + QByteArray::number(expression_data.rowCount()));

		//Set cohort size
		ui_->l_cohort_size->setText("Cohort size: \t " + QString::number(cohort_.size()) + "\nCohort correlation: \t " + QString::number(correlation, 'f', 3));


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

void ExpressionGeneWidget::initBiotypeList()
{
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

QVector<double> ExpressionGeneWidget::calculateRanks(const QVector<double>& values)
{
	QVector<double> sorted_values = values;
	std::sort(sorted_values.rbegin(), sorted_values.rend());
	QVector<double> ranks = QVector<double>(values.size());
	for (int i = 0; i < values.size(); ++i)
	{
		ranks[i] = sorted_values.indexOf(values.at(i)) + 1;
	}
	return ranks;
}



