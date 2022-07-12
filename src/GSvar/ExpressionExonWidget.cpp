#include "ExpressionExonWidget.h"
#include "RepeatExpansionWidget.h"
#include "ui_ExpressionExonWidget.h"

#include "GUIHelper.h"
#include "Helper.h"
#include "NGSD.h"
#include "VersatileFile.h"
#include "LoginManager.h"
#include "BedFile.h"

#include <QMessageBox>
#include <QTime>

ExpressionExonWidget::ExpressionExonWidget(QString tsv_filename, int sys_id, QString tissue, const QString& variant_gene_filter, const GeneSet& variant_gene_set, const QString& project,
										   const QString& ps_id, RnaCohortDeterminationStategy cohort_type, QWidget* parent):
	tsv_filename_(tsv_filename),
	sys_id_(sys_id),
	tissue_(tissue),
	variant_gene_filter_(variant_gene_filter),
	variant_gene_set_(variant_gene_set),
	project_(project),
	ps_id_(ps_id),
	cohort_type_(cohort_type),
	ui_(new Ui::ExpressionExonWidget),
	QWidget(parent)
{
	//debug:
	qDebug() << "filename: " << tsv_filename_;
	qDebug() << "sys: " << NGSD().getProcessingSystemData(sys_id_).name;
	qDebug() << "tissue: " << tissue_;
	qDebug() << "project: " << project_;
	qDebug() << "ps_id: " << NGSD().processedSampleName(ps_id_);

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
	qDebug() << "Init filter";

	//TODO: set default values for filter
	NGSD db;

	//set initial cohort type
	if(cohort_type_ == RNA_COHORT_GERMLINE) ui_->rb_germline->setChecked(true);
	if(cohort_type_ == RNA_COHORT_GERMLINE_PROJECT) ui_->rb_germline_project->setChecked(true);
	if(cohort_type_ == RNA_COHORT_SOMATIC) ui_->rb_somatic->setChecked(true);
	//extract all valid exons from NGSD
	SqlQuery query_exon = db.getQuery();
	query_exon.exec("SELECT DISTINCT gt.chromosome, ge.start, ge.end FROM `gene_exon` ge INNER JOIN `gene_transcript` gt ON ge.transcript_id = gt.id;");
	valid_exons_ = QSet<QByteArray>();
	while(query_exon.next())
	{
		BedLine exon = BedLine(Chromosome("chr" + query_exon.value(0).toString()), query_exon.value(1).toInt(), query_exon.value(2).toInt());
		valid_exons_ << exon.toString(true).toUtf8();
	}
	qDebug() << QByteArray::number(valid_exons_.size()) << " unique exons stored in the NGSD " << endl;

	initBiotypeList();

	qDebug() << "filter initialized";
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

		//db columns
		db_column_names_ = QStringList()  << "cohort_mean" << "log2fc" << "zscore" << "pvalue";
		column_names_ << db_column_names_;
		numeric_columns_  << true << true << true << true;
		precision_ << 2 << 2 << 3 << 3;


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

		QTime timer;
		timer.start();


		// Filter by parameters stored in file

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
			qDebug() << "\t removed: " << (filtered_lines - filter_result_.countPassing()) << Helper::elapsedTime(timer);
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
			qDebug() << "\t removed: " << (filtered_lines - filter_result_.countPassing()) << Helper::elapsedTime(timer);
			filtered_lines = filter_result_.countPassing();
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
		if (!ui_->sb_min_srpb_sample->text().isEmpty())
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

		//filter data
		int idx_biotype = expression_data_.headers().indexOf("gene_biotype");

		for(int row_idx=0; row_idx<expression_data_.rowCount(); ++row_idx)
		{
			//skip already filtered
			if (!filter_result_.flags()[row_idx]) continue;

			QString biotype = expression_data_.row(row_idx).at(idx_biotype);
			filter_result_.flags()[row_idx] = selected_biotypes.contains(biotype.replace("_", " "));
		}

		//debug:
		qDebug() << "\t removed (file based): " << (filtered_lines - filter_result_.countPassing()) << Helper::elapsedTime(timer);
		filtered_lines = filter_result_.countPassing();


		//filter by NGSD stats

		//reset data if cohort changes or on first load
		RnaCohortDeterminationStategy new_cohort_type;
		if (ui_->rb_germline->isChecked())
		{
			new_cohort_type = RNA_COHORT_GERMLINE;
		}
		else if(ui_->rb_germline_project->isChecked())
		{
			new_cohort_type = RNA_COHORT_GERMLINE_PROJECT;
		}
		else //somatic
		{
			new_cohort_type = RNA_COHORT_SOMATIC;
		}


		if((cohort_.size() == 0) || (new_cohort_type != cohort_type_))
		{
			cohort_ = db_.getRNACohort(sys_id_, tissue_, project_, ps_id_, cohort_type_, "exons");
			db_expression_data_.clear();

			//prepare query
			exon_query_ = db_.getQuery();
			//processed sample IDs as string list
			QByteArrayList ps_ids_str;
			foreach (int id, cohort_) ps_ids_str << QByteArray::number(id);
			exon_query_.prepare("SELECT e.srpb FROM expression_exon e WHERE e.chr=:0 AND e.start=:1 AND e.end=:2 AND e.processed_sample_id IN (" + ps_ids_str.join(", ") + "); ");

			cohort_type_ = new_cohort_type;
			qDebug() << "Cohort changed!";
		}

		//filter by cohort srpb value
		if (ui_->sb_min_srpb_cohort->value() != 0.0)
		{
			qDebug() << "filter by cohort srpb";
			double min_cohort_srpb_value = ui_->sb_min_srpb_cohort->value();

			for(int row_idx=0; row_idx<expression_data_.rowCount(); ++row_idx)
			{
				//skip already filtered
				if (!filter_result_.flags()[row_idx]) continue;

				//remove non-valid exons
				BedLine exon = BedLine::fromString(expression_data_.row(row_idx).at(expression_data_.columnIndex("exon")));
				QByteArray exon_str = exon.toString(true).toUtf8();
				if (!valid_exons_.contains(exon_str))
				{
					filter_result_.flags()[row_idx] = false;
					continue;
				}

				//get db expression values for this exon
				getDBExpressionData(row_idx);

				if (db_expression_data_.contains(exon_str))
				{
					filter_result_.flags()[row_idx] = db_expression_data_.value(exon_str).cohort_mean >= min_cohort_srpb_value;
				}
				else
				{
					filter_result_.flags()[row_idx] = false;
				}
			}

			qDebug() << "\t" << Helper::elapsedTime(timer);
		}


				//filter by log fold change
				if (ui_->sb_min_logfc->value() != 0.0)
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



		//update expression table
		updateTable();


		QApplication::restoreOverrideCursor();
	}
	catch (Exception& e)
	{
		GUIHelper::showException(this, e, "Error filtering exon expression file.");
	}
}

void ExpressionExonWidget::copyToClipboard()
{

}

void ExpressionExonWidget::showBiotypeContextMenu(QPoint pos)
{

}

void ExpressionExonWidget::selectAllBiotypes(bool deselect)
{

}

void ExpressionExonWidget::showHistogram(int row_idx)
{

}

void ExpressionExonWidget::showExpressionTableContextMenu(QPoint pos)
{

}

void ExpressionExonWidget::showCohort()
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
		GUIHelper::showException(this, e, "Error displaying cohort.");
	}
}

void ExpressionExonWidget::copyCohortToClipboard()
{
	if (cohort_table_ == nullptr) return;
	GUIHelper::copyToClipboard(cohort_table_);
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
			//skip db columns
			if(db_column_names_.contains(col_name))
			{
				column_indices << -1;
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

			//extract exon name and srpb
			BedLine exon = BedLine::fromString(row.at(expression_data_.columnIndex("exon")));
			QByteArray exon_str = exon.toString(true).toUtf8();


			bool valid_exon = false;
			DBExpressionValues exon_db_values;

			// get exon stats from NGSD for all not-cached exon
			getDBExpressionData(file_line_idx);

			if (valid_exons_.contains(exon_str) && db_expression_data_.contains(exon_str))
			{
				exon_db_values = db_expression_data_.value(exon_str);
				valid_exon = true;
			}


			//iterate over columns
			for (int col_idx = 0; col_idx < column_names_.size(); ++col_idx)
			{
				QByteArray col_name = column_names_.at(col_idx).toUtf8();

				if(db_column_names_.contains(col_name))
				{
					if (valid_exon)
					{
						if(column_names_.at(col_idx) == "cohort_mean")
						{
							ui_->tw_expression_table->setItem(table_row_idx, col_idx, new NumericWidgetItem(QString::number(exon_db_values.cohort_mean, 'f', precision_.at(col_idx))));
						}
						else if(column_names_.at(col_idx) == "log2fc")
						{
							ui_->tw_expression_table->setItem(table_row_idx, col_idx, new NumericWidgetItem(QString::number(exon_db_values.log2fc, 'f', precision_.at(col_idx))));
						}
						else if(column_names_.at(col_idx) == "zscore")
						{
							ui_->tw_expression_table->setItem(table_row_idx, col_idx, new NumericWidgetItem(QString::number(exon_db_values.zscore, 'f', precision_.at(col_idx))));
						}
						else if(column_names_.at(col_idx) == "pvalue")
						{
							ui_->tw_expression_table->setItem(table_row_idx, col_idx, new NumericWidgetItem(QString::number(exon_db_values.pvalue, 'f', precision_.at(col_idx))));
						}
						else
						{
							THROW(ArgumentException, "Invalid db column '" + column_names_.at(col_idx) + "'!");
						}
					}
					else
					{
						ui_->tw_expression_table->setItem(table_row_idx, col_idx, new NumericWidgetItem(""));
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
						ui_->tw_expression_table->setItem(table_row_idx, col_idx, new QTableWidgetItem(row.at(column_indices.at(col_idx))));
					}

					//extract gene biotype
					if (column_names_.at(col_idx) == "gene_biotype")
					{
						//replace '_'
						ui_->tw_expression_table->item(table_row_idx, col_idx)->setText(ui_->tw_expression_table->item(table_row_idx, col_idx)->text().replace('_', ' '));
					}
				}


			}

			if (table_row_idx % 1000 == 0) qDebug() << "Row: " << table_row_idx;
			//update row
			table_row_idx++;
			//abort if max row count is reached
			if (table_row_idx>=max_rows) break;
		}


		//set number of filtered / total rows
		ui_->l_filtered_rows->setText(QByteArray::number(filter_result_.flags().count(true)) + " / " + QByteArray::number(expression_data_.rowCount())
									  + " (" + QByteArray::number(ui_->tw_expression_table->rowCount()) + " displayed) " );

		// activate sorting if all exons are displayed
		if (ui_->tw_expression_table->rowCount() == filter_result_.flags().count(true))
		{
			//enable sorting
			ui_->tw_expression_table->setSortingEnabled(true);

			//sort by zscore on default
	//		ui_->tw_expression_table->sortByColumn(7, Qt::DescendingOrder);
		}


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

void ExpressionExonWidget::getDBExpressionData(int line_idx)
{
	//extract exon name and srpb
	const QStringList& row = expression_data_.row(line_idx);
	BedLine exon = BedLine::fromString(row.at(expression_data_.columnIndex("exon")));
	QByteArray exon_str = exon.toString(true).toUtf8();

	// get exon stats from NGSD for all not-cached exon
	if(valid_exons_.contains(exon_str) && !db_expression_data_.contains(exon_str))
	{
		double srpb, log2p1srpb;
		DBExpressionValues db_expression;

		//execute query
		exon_query_.bindValue(0, exon.chr().strNormalized(true));
		exon_query_.bindValue(1, exon.start());
		exon_query_.bindValue(2, exon.end());
		exon_query_.exec();

		//parse result
		QVector<double> raw_values;
		QVector<double> log2_values;
		while (exon_query_.next())
		{
			double raw = exon_query_.value(0).toDouble();
			double log2p1 = std::log2(raw +1);

			raw_values << raw;
			log2_values << log2p1;
		}

		//calculate statistics for exon
		db_expression.cohort_mean = BasicStatistics::mean(raw_values);
		double mean_log2 = BasicStatistics::mean(log2_values);
		double stddev_log2 = BasicStatistics::stdev(log2_values, mean_log2);

		srpb = Helper::toDouble(row.at(expression_data_.columnIndex("srpb")), "srpb", QString::number(line_idx));
		log2p1srpb = std::log2(srpb + 1);
		db_expression.log2fc = log2p1srpb - std::log2(db_expression.cohort_mean + 1);
		db_expression.zscore = (log2p1srpb - mean_log2) / stddev_log2;
		db_expression.pvalue = 1 + std::erf(- std::abs(db_expression.zscore) / std::sqrt(2));

		db_expression_data_.insert(exon_str, db_expression);
	}


}
