#include "ExpressionDataWidget.h"
#include "ui_ExpressionDataWidget.h"
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



ExpressionDataWidget::ExpressionDataWidget(QString tsv_filename, int sys_id, QString tissue, QWidget *parent) :
	QWidget(parent),
	tsv_filename_(tsv_filename),
	sys_id_(sys_id),
	tissue_(tissue),
	ui_(new Ui::ExpressionDataWidget)

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

	// set context menus for biotype filter
	ui_->sa_biotype->setContextMenuPolicy(Qt::CustomContextMenu);

	loadExpressionData();
}

ExpressionDataWidget::~ExpressionDataWidget()
{
	delete ui_;
}

void ExpressionDataWidget::applyFilters()
{
	//skip if not necessary
	int row_count = ui_->expression_data->rowCount();
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
	QString logfc = ui_->abs_log_fc->text().replace(",", ".");
	if (!logfc.isEmpty())
	{
		int idx = column_names_.indexOf("log2fc");

		if (idx == -1)
		{
			QMessageBox::warning(this, "Filtering error", "Table does not contain a 'log2fc' column! \nFiltering based on fold change is not possible.");
			ui_->abs_log_fc->setText("");
		}
		else
		{
			try
			{
				double logfc_cutoff = Helper::toDouble(logfc);
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
	QString zscore = ui_->abs_zscore->text().replace(",", ".");
	if (!zscore.isEmpty())
	{
		int idx = column_names_.indexOf("zscore");

		if (idx == -1)
		{
			QMessageBox::warning(this, "Filtering error", "Table does not contain a 'zscore' column! \nFiltering based on z-score is not possible.");
			ui_->abs_zscore->setText("");
		}
		else
		{
			try
			{
				double zscore_cutoff = Helper::toDouble(zscore);
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

	//filter by raw counts
	if (!ui_->raw_counts->text().isEmpty())
	{
		int idx = column_names_.indexOf("raw");

		if (idx == -1)
		{
			QMessageBox::warning(this, "Filtering error", "Table does not contain a 'raw' column! \nFiltering based on raw counts is not possible.");
			ui_->raw_counts->setText("");
		}
		else
		{
			try
			{
				int min_raw_count = Helper::toInt(ui_->raw_counts->text(), "Raw count filter entry");
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
						double raw_count = Helper::toDouble(value, "raw count in expression data", QString::number(row_idx+1));
						filter_result.flags()[row_idx] = raw_count >= min_raw_count;
					}
				}
			}
			catch (Exception e)
			{
				QMessageBox::warning(this, "Invalid raw count value", "Couldn't convert given raw count value to number!\n" + e.message());
			}
		}
	}

	//filter by tpm value
	if (!ui_->tpm_value->text().isEmpty())
	{
		int idx = column_names_.indexOf("tpm");

		if (idx == -1)
		{
			QMessageBox::warning(this, "Filtering error", "Table does not contain a 'tpm' column! \nFiltering based on tpm value is not possible.");
		}
		else
		{
			try
			{
				int min_tpm_value = Helper::toDouble(ui_->tpm_value->text().replace(",", "."), "TPM filter entry");
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

	//filter by p-value
	if (!ui_->pvalue->text().isEmpty())
	{
		int idx = column_names_.indexOf("pvalue");

		if (idx == -1)
		{
			QMessageBox::warning(this, "Filtering error", "Table does not contain a 'pvalue' column! \nFiltering based on p-value is not possible.");
		}
		else
		{
			try
			{
				int max_pvalue = Helper::toDouble(ui_->pvalue->text().replace(",", "."), "p-value filter entry");
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
						double pvalue = Helper::toDouble(value);
						filter_result.flags()[row_idx] = pvalue <= max_pvalue;
					}
				}
			}
			catch (Exception e)
			{
				QMessageBox::warning(this, "Invalid p-value", "Couldn't convert given p-value to number!\n" + e.message());
			}
		}
	}

	//filter by biotype
	QSet<QString> selected_biotypes;

	//get selected biotypes
	foreach (QCheckBox* cb_biotype, ui_->sawc_biotype->findChildren<QCheckBox*>())
	{
		if (cb_biotype->isChecked())
		{
			selected_biotypes.insert(cb_biotype->text());
		}
	}

	//filter data
	int idx = column_names_.indexOf("gene_biotype");
	for(int row_idx=0; row_idx<row_count; ++row_idx)
	{
		//skip already filtered
		if (!filter_result.flags()[row_idx]) continue;

		QString biotype = ui_->expression_data->item(row_idx, idx)->text();
		filter_result.flags()[row_idx] = selected_biotypes.contains(biotype);
	}


	//hide rows not passing filters
	for(int row=0; row<row_count; ++row)
	{
		ui_->expression_data->setRowHidden(row, !filter_result.flags()[row]);
	}

	//set number of filtered / total rows
	ui_->filtered_rows->setText(QByteArray::number(filter_result.flags().count(true)) + " / " + QByteArray::number(row_count));

}

void ExpressionDataWidget::copyToClipboard()
{
	GUIHelper::copyToClipboard(ui_->expression_data);
}

void ExpressionDataWidget::showBiotypeContextMenu(QPoint pos)
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

void ExpressionDataWidget::selectAllBiotypes(bool deselect)
{
	//set checked state
	foreach (QCheckBox* cb_biotype, ui_->sawc_biotype->findChildren<QCheckBox*>())
	{
		cb_biotype->setChecked(!deselect);
	}
}

void ExpressionDataWidget::loadExpressionData()
{
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

	//collect biotypes
	QSet<QString> biotypes;

	column_names_.clear();
	numeric_columns_.clear();
	precision_.clear();

	//default columns
	column_names_ << "gene_id" << "gene_name" << "gene_biotype" << "raw" << "tpm";
	numeric_columns_  << false << false << false << true << true;
	precision_ << -1 << -1 << -1 << 0 << 2;

	column_names_ << "log2tpm" << "cohort_mean" << "cohort_meanlog2" << "log2fc" << "zscore";
	numeric_columns_  << true << true << true << true << true;
	precision_ << 2 << 2 << 2 << 2 << 2;

	//determine col indices for table columns in tsv file
	QVector<int> column_indices;
	foreach (const QString& col_name, column_names_)
	{
		column_indices << expression_data.columnIndex(col_name);
	}

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
				ui_->expression_data->setItem(row_idx, col_idx, new QTableWidgetItem(row.at(column_indices.at(col_idx))));
			}

			//extract gene biotype
			if (column_names_.at(col_idx) == "gene_biotype")
			{
				biotypes.insert(row.at(column_indices.at(col_idx)));
			}

		}


	}

	//hide vertical header
	ui_->expression_data->verticalHeader()->setVisible(false);

	//enable sorting
	ui_->expression_data->setSortingEnabled(true);

	//optimize table view
	GUIHelper::resizeTableCells(ui_->expression_data, 200, true, 1000);

	//Set number of filtered / total rows
	ui_->filtered_rows->setText(QByteArray::number(expression_data.rowCount()) + " / " + QByteArray::number(expression_data.rowCount()));

	//init/update filter column
	//biotypes
	QList<QString> sorted_biotypes = biotypes.toList();
	std::sort(sorted_biotypes.begin(), sorted_biotypes.end());
	QVBoxLayout* vbox = new QVBoxLayout;
	foreach (const QString& biotype, sorted_biotypes)
	{
		QCheckBox* cb_biotype = new QCheckBox(biotype);
		cb_biotype->setChecked(true);
		vbox->addWidget(cb_biotype);
	}
	ui_->sawc_biotype->setLayout(vbox);
}




