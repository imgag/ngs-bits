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
#include "GlobalServiceProvider.h"
#include <QChartView>
#include <QDialogButtonBox>
#include <QSignalMapper>
#include <QTextEdit>
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
//	connect(ui_->expression_data->horizontalHeader(), SIGNAL(sectionClicked(int)), this, SLOT(applyFilters()));
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
	connect(ui_->expression_data,SIGNAL(itemDoubleClicked(QTableWidgetItem*)),this,SLOT(OpenInIGV(QTableWidgetItem*)));
	connect(ui_->b_set_custom_cohort, SIGNAL(clicked(bool)), this, SLOT(showCustomCohortDialog()));
	connect(ui_->rb_custom, SIGNAL(toggled(bool)), this, SLOT(toggleUICustomCohort()));
	connect(ui_->rb_germline_tissue, SIGNAL(toggled(bool)), this, SLOT(toggleCohortStats()));
	connect(ui_->rb_germline_project, SIGNAL(toggled(bool)), this, SLOT(toggleCohortStats()));
	connect(ui_->rb_somatic, SIGNAL(toggled(bool)), this, SLOT(toggleCohortStats()));
	connect(ui_->rb_custom, SIGNAL(toggled(bool)), this, SLOT(toggleCohortStats()));
	connect(ui_->cb_sample_quality, SIGNAL(currentIndexChanged(int)), this, SLOT(toggleCohortStats()));



	// set context menus for biotype filter
	ui_->sa_biotype->setContextMenuPolicy(Qt::CustomContextMenu);

	// set initial cohort determination strategy
	if (cohort_type == RNA_COHORT_GERMLINE) ui_->rb_germline_tissue->setChecked(true);
	if (cohort_type == RNA_COHORT_GERMLINE_PROJECT) ui_->rb_germline_project->setChecked(true);
	if (cohort_type == RNA_COHORT_SOMATIC) ui_->rb_somatic->setChecked(true);


	//(de-)activate variant list gene filter
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

	updateCohort();

	initTable();

	applyFilters();


}

ExpressionGeneWidget::~ExpressionGeneWidget()
{
	delete ui_;
}

void ExpressionGeneWidget::applyFilters(int max_rows)
{
	// skip if filtering is runnning
	if (filtering_in_progress_) return;
	filtering_in_progress_ = true;
	//skip if not necessary
	int row_count = expression_data_.rowCount();
	if (row_count == 0) return;

	try
	{
		QApplication::setOverrideCursor(Qt::BusyCursor);
		QTime timer;
		timer.start();
		qDebug() << "applying filter...";

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
		else if (ui_->rb_custom->isChecked())
		{
			cohort_type = RNA_COHORT_CUSTOM;
		}
		else
		{
			THROW(ArgumentException, "Invalid cohort type!");
		}

		QStringList exclude_quality = getQualityFilter();

		if ((cohort_type != cohort_type_) || (exclude_quality != exclude_quality_) || ((cohort_type == RNA_COHORT_CUSTOM) && (cohort_ != custom_cohort_)))
		{
			//update cohort determination strategy
			cohort_type_ = cohort_type;
			exclude_quality_ = exclude_quality;
			updateCohort();
		}

		filter_result_.reset();

		//filter based on file information
		int gene_idx = expression_data_.columnIndex("gene_name");

		//filter by variant list gene filter
		if (!variant_gene_set_.isEmpty() && (ui_->cb_filter_by_var_list->checkState() == Qt::Checked))
		{
			qDebug() << "filter by variant gene filter";

			for(int row_idx=0; row_idx<row_count; ++row_idx)
			{
				if (!filter_result_.flags()[row_idx]) continue;

				filter_result_.flags()[row_idx] = variant_gene_set_.contains(expression_data_.row(row_idx).at(gene_idx).toUtf8().trimmed());
			}

			qDebug() << filter_result_.countPassing();
		}

		//filter by genes
		GeneSet gene_whitelist = GeneSet::createFromText(ui_->gene_filter->text().toUtf8(), ',');
		if (!gene_whitelist.isEmpty())
		{
			qDebug() << "filter by gene filter";
			QByteArray genes_joined = gene_whitelist.join('|');

			if (genes_joined.contains("*")) //with wildcards
			{
				QRegExp reg(genes_joined.replace("-", "\\-").replace("*", "[A-Z0-9-]*"));
				for(int row_idx=0; row_idx<row_count; ++row_idx)
				{
					if (!filter_result_.flags()[row_idx]) continue;

					// generate GeneSet from column text
					GeneSet sv_genes = GeneSet::createFromText(expression_data_.row(row_idx).at(gene_idx).toUtf8().trimmed(), ',');

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
				for(int row_idx=0; row_idx<row_count; ++row_idx)
				{
					if (!filter_result_.flags()[row_idx]) continue;

					// generate GeneSet from column text
					GeneSet sv_genes = GeneSet::createFromText(expression_data_.row(row_idx).at(gene_idx).toUtf8().trimmed(), ',');

					filter_result_.flags()[row_idx] = sv_genes.intersectsWith(gene_whitelist);
				}
			}
			qDebug() << filter_result_.countPassing();
		}

		//filter by tpm value
		if (ui_->tpm_value->value() != 0.0)
		{
			qDebug() << "filter by tpm";
			int tpm_idx = expression_data_.columnIndex("tpm");

			double min_tpm_value = ui_->tpm_value->value();
			for(int row_idx=0; row_idx<row_count; ++row_idx)
			{
				//skip already filtered
				if (!filter_result_.flags()[row_idx]) continue;

				QString value = expression_data_.row(row_idx).at(tpm_idx).trimmed();
				if (value.isEmpty() || value == "n/a")
				{
					filter_result_.flags()[row_idx] = false;
				}
				else
				{
					double tpm_value = Helper::toDouble(value);
					filter_result_.flags()[row_idx] = tpm_value >= min_tpm_value;
				}
			}
			qDebug() << filter_result_.countPassing();
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
		int idx_biotype = expression_data_.columnIndex("gene_biotype");


		for(int row_idx=0; row_idx<row_count; ++row_idx)
		{
			//skip already filtered
			if (!filter_result_.flags()[row_idx]) continue;

			QString biotype = expression_data_.row(row_idx).at(idx_biotype).trimmed().replace("_", " ");
			filter_result_.flags()[row_idx] = selected_biotypes.contains(biotype);
		}

		//filter based on low expression tpm value (1st part (file-based)
		if (ui_->low_expr_tpm->value() != 0.0)
		{
			qDebug() << "filter by low expression (1st part)";

			double low_expr_tpm = ui_->low_expr_tpm->value();
			int tpm_idx = expression_data_.columnIndex("tpm");

			for(int row_idx=0; row_idx<row_count; ++row_idx)
			{
				//skip already filtered
				if (!filter_result_.flags()[row_idx]) continue;

				QString value_sample = expression_data_.row(row_idx).at(tpm_idx);
				if (value_sample.isEmpty() || value_sample == "n/a")
				{
					filter_result_.flags()[row_idx] = false;
				}
				else
				{
					double tpm_sample = Helper::toDouble(value_sample);
					filter_result_.flags()[row_idx] = tpm_sample >= low_expr_tpm;
				}
			}
		}

		qDebug() << filter_result_.countPassing();

		//filter based on NGSD information
		if ((ui_->abs_logfc->value() != 0.0) || (ui_->abs_zscore->value() != 0.0) || (ui_->tpm_cohort_value->value() != 0.0) || (ui_->low_expr_tpm->value() != 0.0))
		{
			// check for valid cohort
			if(cohort_.size() == 0) THROW(ArgumentException, "Selected cohort does not contain any samples! Cannot filter based on NGSD cohort data.");

			qDebug() << "Filter by NGSD columns";
			int gene_id_idx = expression_data_.columnIndex("gene_id");
			int tpm_idx = expression_data_.columnIndex("tpm");


			//get cut-offs
			double logfc_cutoff = ui_->abs_logfc->value();
			double zscore_cutoff = ui_->abs_zscore->value();
			double min_cohort_tpm_value = ui_->tpm_cohort_value->value();
			double low_expr_tpm = ui_->low_expr_tpm->value();

			//count number of kept rows
			int n_kept_rows = 0;

			for(int row_idx=0; row_idx<row_count; ++row_idx)
			{
				//skip already filtered
				if (!filter_result_.flags()[row_idx]) continue;

				//get gene and tpm value
				bool in_db = false;
				QByteArray ensg_number = expression_data_.row(row_idx).at(gene_id_idx).toUtf8().trimmed();
				QByteArray gene_symbol;

				if(ensg_mapping_.contains(ensg_number))
				{
					gene_symbol = ensg_mapping_.value(ensg_number);
				}
				if(gene_symbol.isEmpty())
				{
					filter_result_.flags()[row_idx] = false;
					continue;
				}

				double tpm = 0.0;
				QString value = expression_data_.row(row_idx).at(tpm_idx).toUtf8().trimmed();
				if (!(value.isEmpty() || value == "n/a"))
				{
					tpm = Helper::toDouble(value);
				}

				//get stats from database
				DBExpressionValues db_values;
				if (!ngsd_expression.contains(gene_symbol)) getGeneStats(gene_symbol, tpm);
				if (ngsd_expression.contains(gene_symbol))
				{
					db_values = ngsd_expression.value(gene_symbol);
					in_db = true;
				}


				//filter by log fold change
				if (logfc_cutoff != 0.0)
				{
//					qDebug() << "filter by log fc";
					if (!in_db)
					{
						filter_result_.flags()[row_idx] = false;
						continue;
					}

					if(fabs(db_values.log2fc) <= logfc_cutoff)
					{
						filter_result_.flags()[row_idx] = false;
						continue;
					}

				}

				//filter by cohort z-score
				if (zscore_cutoff != 0.0)
				{
//					qDebug() << "filter by zscore";
					if (!in_db)
					{
						filter_result_.flags()[row_idx] = false;
						continue;
					}

					if(fabs(db_values.zscore) <= zscore_cutoff)
					{
						filter_result_.flags()[row_idx] = false;
						continue;
					}
				}

				//filter by cohort tpm value
				if (min_cohort_tpm_value != 0.0)
				{
//					qDebug() << "filter by cohort mean";
					if (!in_db)
					{
						filter_result_.flags()[row_idx] = false;
						continue;
					}

					if(db_values.cohort_mean <= min_cohort_tpm_value)
					{
						filter_result_.flags()[row_idx] = false;
						continue;
					}
				}

				//filter by low expression tpm value (2nd part (db-based))
				if (low_expr_tpm != 0.0)
				{
//					qDebug() << "filter by low expression";
					if (!in_db)
					{
						filter_result_.flags()[row_idx] = false;
						continue;
					}

					if(db_values.cohort_mean <= low_expr_tpm)
					{
						filter_result_.flags()[row_idx] = false;
						continue;
					}
				}

				//row passed all filters
				n_kept_rows++;


				//stop filtering if enough rows pass filter
				if(n_kept_rows > max_rows) break;

			}
		}

		qDebug() << "Filtering took " << Helper::elapsedTime(timer);
		qDebug() << "Remaining rows: " << filter_result_.countPassing();
		qDebug() << "Gene cache size: " << ngsd_expression.size();

		//update table
		updateTable(max_rows);

		//update GUI
		toggleCohortStats(true);

		QApplication::restoreOverrideCursor();
		filtering_in_progress_ = false;


	}
	catch (Exception& e)
	{
		GUIHelper::showException(this, e, "Error filtering RNA expression file.");
		filtering_in_progress_ = false;
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
	QByteArray ensg = ui_->expression_data->item(row_idx, 0)->text().toUtf8();
	QVector<double> expr_values = db_.getGeneExpressionValues(db_.getEnsemblGeneMapping().value(ensg), cohort_, false);
	double tpm = ui_->expression_data->item(row_idx, 4)->text().toDouble();

	if(expr_values.size() == 0) return;
	//create histogram
	std::sort(expr_values.begin(), expr_values.end()); 
	double max = expr_values.constLast();
	Histogram hist(0.0, max, max/40);
	foreach(double expr_value, expr_values)
	{
		hist.inc(expr_value, true);
	}

	//show chart
	QChartView* view = GUIHelper::histogramChart(hist, "Expression value distribution (TPM, " + QString::number(expr_values.size()) + " samples)", hist.binIndex(tpm, true));
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

void ExpressionGeneWidget::OpenInIGV(QTableWidgetItem* item)
{
	if (item==nullptr) return;

	int gene_col_idx = column_names_.indexOf("gene_name");
	int row_idx = item->row();

	QString gene_name = ui_->expression_data->item(row_idx, gene_col_idx)->text();

	GlobalServiceProvider::gotoInIGV(gene_name, true);
}

void ExpressionGeneWidget::showCustomCohortDialog()
{
	if (!LoginManager::active()) return;
	NGSD db;
	try
	{
		qDebug() << "create CustomCohortDialog";
		QDialog custom_cohort_dialog(this);
		custom_cohort_dialog.setWindowFlags(Qt::Window);
		custom_cohort_dialog.setWindowTitle("Set custom cohort of Sample " + db.processedSampleName(ps_id_));
		custom_cohort_dialog.setLayout(new QBoxLayout(QBoxLayout::TopToBottom));
		custom_cohort_dialog.layout()->setMargin(3);

		//add description:
		QLabel* description = new QLabel("Define the custom cohort by adding all processed sample which should be part of the cohort (separated by new lines):");
		custom_cohort_dialog.layout()->addWidget(description);

		//add table
		QTextEdit* cohort_text = new QTextEdit();
		custom_cohort_dialog.layout()->addWidget(cohort_text);

		//init with already determined samples:
		QStringList processed_sample_names;
		foreach (int ps_id, custom_cohort_)
		{
			processed_sample_names.append(db.processedSampleName(QString::number(ps_id)));
		}
		cohort_text->setText(processed_sample_names.join('\n'));

		//add buttons
		QDialogButtonBox* buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
		custom_cohort_dialog.layout()->addWidget(buttonBox);
		connect(buttonBox, SIGNAL(accepted()), &custom_cohort_dialog, SLOT(accept()));
		connect(buttonBox, SIGNAL(rejected()), &custom_cohort_dialog, SLOT(reject()));

		//show dialog
		int result = custom_cohort_dialog.exec();

		if(result == QDialog::Accepted)
		{
			//parse results
			processed_sample_names = cohort_text->toPlainText().replace(",", "\n").replace(";", "\n").replace(" ", "\n").split('\n');
			QSet<int> new_custom_cohort;
			QStringList missing_samples;
			foreach (const QString& ps_name, processed_sample_names)
			{
				//skip empty lines
				if(ps_name.trimmed().isEmpty()) continue;
				QString ps_id = db.processedSampleId(ps_name, false);
				if(ps_id.isEmpty())
				{
					missing_samples.append(ps_name);
					continue;
				}
				new_custom_cohort.insert(ps_id.toInt());
			}

			if(missing_samples.size() != 0)
			{
				GUIHelper::showMessage("Missing samples", "The following processed samples are not found in the NGSD:\n" + missing_samples.join(",\n"));
			}
			custom_cohort_ = new_custom_cohort;
		}
	}
	catch (Exception& e)
	{
		GUIHelper::showException(this, e, "Error opening RNA expression file.");
	}
}

void ExpressionGeneWidget::toggleUICustomCohort()
{
	ui_->b_set_custom_cohort->setEnabled(ui_->rb_custom->isChecked());
	ui_->cb_sample_quality->setEnabled(!ui_->rb_custom->isChecked());
}

void ExpressionGeneWidget::toggleCohortStats(bool enable)
{
	ui_->l_cohort_size->setEnabled(enable);
	ui_->show_cohort->setEnabled(enable);
}

void ExpressionGeneWidget::updateCohort()
{
	if(cohort_type_ == RNA_COHORT_CUSTOM)
	{
		if(custom_cohort_.size() == 0)
		{
			// open Dialog to set sample list
			showCustomCohortDialog();
		}
		cohort_ = custom_cohort_;
	}
	else
	{
		cohort_ = db_.getRNACohort(sys_id_, tissue_, project_, ps_id_, cohort_type_, "genes", exclude_quality_, false);
	}

	//update NGSD query
	updateQuery();

	//reset cache
	ngsd_expression.clear();
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
		expression_data_ = TsvFile();
		QSharedPointer<VersatileFile> expression_data_file = Helper::openVersatileFileForReading(tsv_filename_, false);

		//parse TSV file
		while (!expression_data_file->atEnd())
		{
			QString line = expression_data_file->readLine().replace("\r", "").replace("\n", "");
			if (line == "")
			{
				// skip empty lines
				continue;
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


		//init filter mask
		filter_result_ = FilterResult(expression_data_.rowCount());

		QApplication::restoreOverrideCursor();
	}
	catch (Exception& e)
	{
		GUIHelper::showException(this, e, "Error opening RNA expression file.");
	}

}

void ExpressionGeneWidget::initTable()
{
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
	db_column_names_ = QStringList()  << "cohort_mean" << "log2fc" << "zscore" << "pval";
	column_names_ << db_column_names_;
	numeric_columns_  << true << true << true << true;
	precision_ << 2 << 2 << 3 << 3;

	//add hpa columns if available
	QStringList headers = expression_data_.headers();
	if (headers.contains("hpa_tissue_tpm"))
	{
		column_names_ << "hpa_tissue_tpm" << "hpa_tissue_log2tpm" << "hpa_sample_log2tpm" << "hpa_log2fc";
		numeric_columns_  << true << true << true << true;
		precision_ << 3 << 3 << 3 << 3;
	}

	//create header
	ui_->expression_data->setColumnCount(column_names_.size());
	for (int col_idx = 0; col_idx < column_names_.size(); ++col_idx)
	{
		ui_->expression_data->setHorizontalHeaderItem(col_idx, new QTableWidgetItem(column_names_.at(col_idx)));
	}

	//init db ensg gene mapping
	ensg_mapping_ = db_.getEnsemblGeneMapping();

}

void ExpressionGeneWidget::updateQuery()
{
	query_gene_stats_ = db_.getQuery();
	QByteArrayList cohort_id_list;
	foreach (int id, cohort_)
	{
		cohort_id_list << QByteArray::number(id);
	}
	query_gene_stats_.prepare(QString() + "SELECT AVG(e.tpm), AVG(LOG2(e.tpm+1)), STD(LOG2(e.tpm+1)) FROM expression e "
							  + "WHERE e.processed_sample_id IN (" + cohort_id_list.join(", ") + ") AND e.symbol=:0;");
}


void ExpressionGeneWidget::updateTable(int max_rows)
{
	try
	{
		QApplication::setOverrideCursor(Qt::BusyCursor);
		QTime timer;
		timer.start();
		qDebug() << "update table...";

		//disable sorting
		ui_->expression_data->setSortingEnabled(false);

		//clear table
		ui_->expression_data->setRowCount(0);


		//set table size
		ui_->expression_data->setRowCount(std::min(max_rows, filter_result_.countPassing()));

		int gene_id_idx = expression_data_.columnIndex("gene_id");
		int table_row_idx = 0;

		//determine col indices for table columns in tsv file
		QVector<int> column_indices;
		foreach (const QString& col_name, column_names_)
		{
			if(expression_data_.headers().contains(col_name))
			{
				column_indices << expression_data_.columnIndex(col_name);
			}
			else
			{
				column_indices << -1;
			}
		}


		for(int file_row_idx=0; file_row_idx<expression_data_.rowCount(); ++file_row_idx)
		{
			//skip rows which are filtered out
			if(!filter_result_.flags()[file_row_idx]) continue;

			QStringList row = expression_data_.row(file_row_idx);
			QByteArray ensg_number = row.at(gene_id_idx).toUtf8();
			QByteArray gene;
			if (ensg_mapping_.contains(ensg_number)) gene = ensg_mapping_.value(ensg_number);
			bool in_db = false;
			DBExpressionValues db_values;
			if (!gene.isEmpty() && ngsd_expression.contains(gene))
			{
				db_values = ngsd_expression.value(gene);
				in_db = true;
			}


			for (int col_idx = 0; col_idx < column_names_.size(); ++col_idx)
			{
				QString col_name = column_names_.at(col_idx);

				if(db_column_names_.contains(col_name))
				{
					if(in_db)
					{

						//db_columns
						if(col_name == "cohort_mean")
						{
							ui_->expression_data->setItem(table_row_idx, col_idx, new NumericWidgetItem(QString::number(db_values.cohort_mean, 'f', precision_.at(col_idx))));
						}
						else if(col_name == "log2fc")
						{
							ui_->expression_data->setItem(table_row_idx, col_idx, new NumericWidgetItem(QString::number(db_values.log2fc, 'f', precision_.at(col_idx))));
						}
						else if(col_name == "zscore")
						{
							ui_->expression_data->setItem(table_row_idx, col_idx, new NumericWidgetItem(QString::number(db_values.zscore, 'f', precision_.at(col_idx))));
						}
						else if(col_name == "pval")
						{
							ui_->expression_data->setItem(table_row_idx, col_idx, new NumericWidgetItem(QString::number(db_values.pvalue, 'f', precision_.at(col_idx))));
						}
						else
						{
							THROW(ArgumentException, "Invalid db column '" + col_name + "'!");
						}
					}
					else
					{
						ui_->expression_data->setItem(table_row_idx, col_idx, new NumericWidgetItem(""));
					}

				}
				else
				{
					//file columns

					//get value from file
					if(numeric_columns_.at(col_idx))
					{
						// add numeric QTableWidgetItem
						QString value = row.at(column_indices.at(col_idx));
						if (value != "n/a" && !value.isEmpty())
						{
							QString rounded_number = QString::number(Helper::toDouble(value,
																					  "TSV column " + QString::number(col_idx),
																					  QString::number(table_row_idx)), 'f', precision_.at(col_idx));
							ui_->expression_data->setItem(table_row_idx, col_idx, new NumericWidgetItem(rounded_number));
						}
						else
						{
							ui_->expression_data->setItem(table_row_idx, col_idx, new NumericWidgetItem(""));
						}
					}
					else
					{
						// add standard QTableWidgetItem
						QTableWidgetItem* cell = new QTableWidgetItem(row.at(column_indices.at(col_idx)));
						cell->setFlags(cell->flags() &  ~Qt::ItemIsEditable);
						ui_->expression_data->setItem(table_row_idx, col_idx, cell);
					}

				}

			}

			table_row_idx++;

			if (table_row_idx >= max_rows) break;
		}

		//enable sorting
		ui_->expression_data->setSortingEnabled(true);

		//sort by zscore on default
		ui_->expression_data->sortByColumn(7, Qt::DescendingOrder);

		//optimize table view
		GUIHelper::resizeTableCells(ui_->expression_data, 200, true, 1000);

		//Set number of filtered / total rows
		if (filter_result_.countPassing() >= max_rows)
		{
			ui_->filtered_rows->setText(QByteArray::number(max_rows) + "+ / " + QByteArray::number(expression_data_.rowCount()) + " (showing only first " + QByteArray::number(max_rows) + ")");
		}
		else
		{
			ui_->filtered_rows->setText(QByteArray::number(ui_->expression_data->rowCount()) + " / " + QByteArray::number(expression_data_.rowCount()));
		}


		//Set cohort size
		ui_->l_cohort_size->setText("Cohort size: \t " + QString::number(cohort_.size()));


		qDebug() << QString() + "... done(" + Helper::elapsedTime(timer) + ")";

		QApplication::restoreOverrideCursor();

	}
	catch (Exception& e)
	{
		GUIHelper::showException(this, e, "Error updating expression table.");
	}
}

void ExpressionGeneWidget::initBiotypeList()
{
	//biotypes
	QStringList sorted_biotypes = db_.getEnum("gene_transcript", "biotype");
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

bool ExpressionGeneWidget::getGeneStats(const QByteArray& gene, double tpm)
{
	query_gene_stats_.bindValue(0, gene);
	query_gene_stats_.exec();
	if(query_gene_stats_.size() > 0)
	{
		query_gene_stats_.next();

		double mean = query_gene_stats_.value(0).toDouble();
		double mean_log2 = query_gene_stats_.value(1).toDouble();
		double stddev_log2 = query_gene_stats_.value(2).toDouble();
		double log2p1tpm = std::log2(tpm + 1);

		DBExpressionValues db_expression_values;
		db_expression_values.cohort_mean = mean;
		db_expression_values.log2fc = log2p1tpm - mean_log2;
		db_expression_values.zscore = std::numeric_limits<double>::quiet_NaN();
		if (stddev_log2 != 0.0) db_expression_values.zscore = (log2p1tpm - mean_log2) / stddev_log2;
		db_expression_values.pvalue = 1 + std::erf(- std::abs(db_expression_values.zscore) / std::sqrt(2));

		ngsd_expression.insert(gene, db_expression_values);
		return true;
	}
	return false;
}

QStringList ExpressionGeneWidget::getQualityFilter()
{
	QStringList exclude_quality;
	if(ui_->cb_sample_quality->currentText() == "Remove samples with quality 'bad'")
	{
		exclude_quality << "bad";
	}
	else if(ui_->cb_sample_quality->currentText() == "Select samples with quality 'good' and 'medium'")
	{
		exclude_quality << "bad" << "n/a";
	}
	else if(ui_->cb_sample_quality->currentText() == "Select samples with quality 'good'")
	{
		exclude_quality << "medium" << "bad" << "n/a";
	}
	//else: select all samples

	return exclude_quality;
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




