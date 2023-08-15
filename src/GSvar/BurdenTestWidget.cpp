#include "BurdenTestWidget.h"
#include "ui_BurdenTestWidget.h"
#include "GlobalServiceProvider.h"
#include <GUIHelper.h>
#include <LoginManager.h>
#include <NGSD.h>
#include <QDialog>
#include <QFileDialog>
#include <QMessageBox>
#include <QTextEdit>


BurdenTestWidget::BurdenTestWidget(QWidget *parent) :
	QWidget(parent),
	ui_(new Ui::BurdenTestWidget)
{
	if (!LoginManager::active())
	{
		INFO(DatabaseException, "Burden Test Widget requires logging in into NGSD!");
	}

	LoginManager::checkRoleNotIn(QStringList{"user_restricted"});

	ui_->setupUi(this);

	//connect signals and slots
	connect(ui_->b_load_samples_cases, SIGNAL(clicked(bool)), this, SLOT(loadCaseSamples()));
	connect(ui_->b_load_samples_controls, SIGNAL(clicked(bool)), this, SLOT(loadControlSamples()));
	connect(ui_->b_burden_test, SIGNAL(clicked(bool)), this, SLOT(performBurdenTest()));
	connect(ui_->b_load_genes, SIGNAL(clicked(bool)), this, SLOT(loadGeneList()));
	connect(ui_->b_add_excluded_regions, SIGNAL(clicked(bool)), this, SLOT(loadExcludedRegions()));
	connect(ui_->b_clear_excluded_regions, SIGNAL(clicked(bool)), this, SLOT(clearExcludedRegions()));
	connect(ui_->b_copy_clipboard, SIGNAL(clicked(bool)), this, SLOT(copyToClipboard()));
	connect(ui_->b_check_input, SIGNAL(clicked(bool)), this, SLOT(validateInputData()));

	//initial GUI setup:
	//disable test start until data is validated
	ui_->b_burden_test->setEnabled(false);
}

BurdenTestWidget::~BurdenTestWidget()
{
	delete ui_;
}

void BurdenTestWidget::loadCaseSamples()
{
	QSet<int> ps_ids = loadSampleList("cases", case_samples_);
	if(ps_ids.size() > 0) case_samples_ = ps_ids;
	cases_initialized_ = (case_samples_.size() > 0);

	qDebug() << "Case:" << case_samples_.size() << case_samples_;

	updateSampleCounts();
	//disable test start until data is validated
	ui_->b_burden_test->setEnabled(false);
}

void BurdenTestWidget::loadControlSamples()
{
	QSet<int> ps_ids = loadSampleList("controls", control_samples_);
	if(ps_ids.size() > 0) control_samples_ = ps_ids;
	controls_initialized_ = (control_samples_.size() > 0);

	qDebug() << "Control:" << control_samples_.size() << control_samples_;

	updateSampleCounts();
	//disable test start until data is validated
	ui_->b_burden_test->setEnabled(false);
}

void BurdenTestWidget::loadGeneList()
{
	//create dialog
	QTextEdit* te_genes = new QTextEdit();
	if(selected_genes_.count() > 0)
	{
		te_genes->setText(selected_genes_.toStringList().join("\n"));
	}
	QSharedPointer<QDialog> dialog = GUIHelper::createDialog(te_genes, "Gene selection",
															 "Paste the gene list (one gene per line).", true);

	//show dialog
	if(dialog->exec()!=QDialog::Accepted) return;

	//parse text field
	GeneSet selected_genes = GeneSet::createFromText(te_genes->toPlainText().toUtf8());

	selected_genes = db_.genesToApproved(selected_genes);

	//apply new gene set
	selected_genes_ = selected_genes;

	gene_set_initialized_ = true;

	updateGeneCounts();
}

void BurdenTestWidget::loadExcludedRegions()
{
	//create dialog
	te_excluded_regions_ = new QTextEdit();
	te_excluded_regions_->setMinimumWidth(640);
	te_excluded_regions_->setMinimumHeight(480);
	QPushButton* b_open_bed_file = new QPushButton("Open BED file");
	QSpacerItem* h_spacer = new QSpacerItem(40, 20, QSizePolicy::Expanding);
	QHBoxLayout* h_box = new QHBoxLayout();
	h_box->addWidget(b_open_bed_file);
	h_box->addItem(h_spacer);
	QVBoxLayout* v_box = new QVBoxLayout();
	v_box->addWidget(te_excluded_regions_);
	v_box->addItem(h_box);
	QWidget* dlg = new QWidget();
	dlg->setLayout(v_box);
	connect(b_open_bed_file, SIGNAL(clicked(bool)), this, SLOT(loadBedFile()));
	QSharedPointer<QDialog> dialog = GUIHelper::createDialog(dlg, "Excluded region selection",
															 "Paste a list of regions which should be added to the list of excluded regions for the varaint search (BED format).", true);

	//show dialog
	if(dialog->exec()!=QDialog::Accepted) return;

	try
	{
		BedFile excluded_region = BedFile::fromText(te_excluded_regions_->toPlainText().toUtf8());
		excluded_region.clearAnnotations();
		excluded_region.clearHeaders();

		//get previous counts
		int prev_region_counts = excluded_regions_.count();
		float prev_base_count = (float) excluded_regions_.baseCount()/1000000.0;

		//apply new exclusion
		excluded_regions_.add(excluded_region);
		excluded_regions_.sort();
		excluded_regions_.merge();

		//update counts
		updateExcludedRegions();

		//show info message
		QMessageBox::information(this, "Excluded regions extended", "The excluded regions have been extended from " + QString::number(prev_region_counts) + " regions ("
								 + QString::number(prev_base_count, 'f', 2) + " Mb) to " + QString::number(excluded_regions_.count()) + " regions ("
								 + QString::number((float) excluded_regions_.baseCount()/1000000.0, 'f', 2) + " Mb).");

	}
	catch (Exception e)
	{
		QMessageBox::critical(this, "BED file parsing failed!", "Parsing of provided regions failed:\n\n" + e.message());
	}
}

void BurdenTestWidget::clearExcludedRegions()
{
	excluded_regions_.clear();
	updateExcludedRegions();
	QMessageBox::information(this, "Excluded regions cleared", "The excluded regions have been removed.");
}

void BurdenTestWidget::loadBedFile()
{
	try
	{

		const QString& init_path = Settings::path("burden_test_files");
		QString bed_file_path = QFileDialog::getOpenFileName(this, "Open BED file", init_path, "BED files (*.bed)");
		BedFile excluded_regions;
		excluded_regions.load(bed_file_path);
		te_excluded_regions_->setText(excluded_regions.toText());
	}
	catch (Exception e)
	{
		QMessageBox::critical(this, "BED file opening failed!", "Opening of BED file failed:\n\n" + e.message());
	}
}

void BurdenTestWidget::validateInputData()
{
	QStringList errors;
	QStringList warnings;

	// check overlap
	QSet<int> overlap = case_samples_ & control_samples_;
	if(overlap.size() > 0)
	{
		QStringList ps_names;
		foreach (int ps_id, overlap)
		{
			ps_names << db_.processedSampleName(QString::number(ps_id));
		}
		errors << "ERROR: The samples " + ps_names.join(", ") + " are present in both case and control cohort!";
	}

	//check same processing system and ancestry
	QSet<int> ps_ids = case_samples_ + control_samples_;
	QSet<QString> processing_systems;
	QSet<QString> ancestry;
	QMap<int,int> sample_ids;
	foreach (int ps_id, ps_ids)
	{
		QString ps_name = db_.processedSampleName(QString::number(ps_id));
		ProcessedSampleData ps_info = db_.getProcessedSampleData(QString::number(ps_id));
		processing_systems.insert(ps_info.processing_system);
		ancestry.insert(ps_info.ancestry);

		sample_ids[db_.sampleId(ps_name).toInt()]++;
	}
	//check for duplicate samples
	QSet<int> sample_id_set = sample_ids.keys().toSet();
	foreach (int s_id, sample_ids.keys())
	{
		if (sample_ids.value(s_id) > 1)
		{
			warnings << "WARNING: Sample " + db_.sampleName(QString::number(s_id)) + " ocurres multiple times in cohort!";
		}
		//check for same sample relations
		QSet<int> same_sample_overlap = db_.sameSamples(s_id) & sample_id_set;
		if (same_sample_overlap.size() > 0)
		{
			QStringList same_sample_list;
			foreach (int id, same_sample_overlap)
			{
				same_sample_list << db_.sampleName(QString::number(id));
			}
			warnings << "WARNING: Sample " + db_.sampleName(QString::number(s_id)) + " has a same-sample/same-patient relation with the sample(s) " + same_sample_list.join(", ") + " in the cohort!";
		}
		//check for related samples
		QStringList relation_types = QStringList() << "parent-child" << "siblings" << "cousins" << "twins" << "twins (monozygotic)";
		foreach (const QString& relation, relation_types)
		{
			QSet<int> related_sample_overlap = db_.relatedSamples(s_id, relation) & sample_id_set;
			if (related_sample_overlap.size() > 0)
			{
				QStringList related_sample_list;
				foreach (int id, related_sample_overlap)
				{
					related_sample_list << db_.sampleName(QString::number(id));
				}
				warnings << "WARNING: Sample " + db_.sampleName(QString::number(s_id)) + " has a " + relation + " relation with the sample(s) " + related_sample_list.join(", ") + " in the cohort!";
			}
		}
	}


	if (processing_systems.size() > 1) warnings << "WARNING: The cohort contains multiple processing systems (" + processing_systems.toList().join(", ")+ ")!";
	if (processing_systems.size() > 1) warnings << "WARNING: The cohort contains multiple ancestries (" + ancestry.toList().join(", ")+ ")!";

	if(errors.size() > 0)
	{
		QMessageBox::warning(this, "Validation error", "During cohort validation the following errors occured:\n" +  errors.join("\n"));
		ui_->b_burden_test->setEnabled(false);
		return;
	}
	else
	{
		// enable if control and case samples are set
		ui_->b_burden_test->setEnabled(cases_initialized_ && controls_initialized_ && (selected_genes_.count() > 0));
	}

	if(warnings.size() > 0)
	{
		//create dialog
		QTextEdit* te_warnings = new QTextEdit();
		te_warnings->setText(warnings.join("\n"));
		te_warnings->setReadOnly(true);
		te_warnings->setMinimumWidth(720);
		te_warnings->setMinimumHeight(480);
		QSharedPointer<QDialog> dialog = GUIHelper::createDialog(te_warnings, "Validation warning", "During cohort validation the following problems occured:\n", true);

		//show dialog
		dialog->exec();
	}

	qDebug() << "Case:" << case_samples_.size() << case_samples_;
	qDebug() << "Control:" << control_samples_.size() << control_samples_;
}

void BurdenTestWidget::updateSampleCounts()
{
	ui_->l_cases->setText(QString::number(case_samples_.size()) + " samples");
	ui_->l_controls->setText(QString::number(control_samples_.size()) + " samples");
}


void BurdenTestWidget::updateGeneCounts()
{
	ui_->l_gene_count->setText(QString::number(selected_genes_.count()) + " genes");
	//disable test start until data is validated
	ui_->b_burden_test->setEnabled(false);
}


void BurdenTestWidget::updateExcludedRegions()
{
	ui_->l_excluded_regions->setText(QString::number(excluded_regions_.count()) + " regions (" + QString::number(((float) excluded_regions_.baseCount()/1000000.0), 'f', 2) + " Mb)");
	//disable test start until data is validated
	ui_->b_burden_test->setEnabled(false);
}


QSet<int> BurdenTestWidget::loadSampleList(const QString& type, const QSet<int>& selected_ps_ids)
{
	NGSD db;

	//create dialog
	QTextEdit* te_samples = new QTextEdit();
	QStringList preselection;
	if(selected_ps_ids.size() > 0)
	{
		foreach (int ps_id, selected_ps_ids)
		{
			preselection << db.processedSampleName(QString::number(ps_id));
		}
		std::sort(preselection.begin(), preselection.end());

		te_samples->setText(preselection.join("\n"));
	}
	QSharedPointer<QDialog> dialog = GUIHelper::createDialog(te_samples, "Sample selection (" +  type +")",
															 "Paste the processed sample list. Either as one sample per line or as comma seperated list.", true);

	//show dialog
	if(dialog->exec()!=QDialog::Accepted) return QSet<int>();

	//parse text field
	QStringList samples;
	QStringList text = te_samples->toPlainText().split('\n');
	if(text.size() == 1)
	{
		//return empty list
		if(text.at(0).trimmed().isEmpty()) return QSet<int>();

		samples = text[0].replace(";", ",").split(',');
	}
	else
	{
		foreach (const QString& line, text)
		{
			if(line.startsWith("#") || line.trimmed().isEmpty()) continue;
			samples << line.split('\t').at(0).trimmed();

			qDebug() << "Sample: " << line;
		}
	}

	//convert ps names to ids and check existence in NGSD
	QSet<int> ps_ids;
	QStringList invalid_samples;
	foreach (const QString& ps_name, samples)
	{
		QString ps_id = db.processedSampleId(ps_name, false);
		if(ps_id.isEmpty())
		{
			invalid_samples.append(ps_name);

			qDebug() << ps_name << "not found";
		}
		else
		{
			ps_ids << ps_id.toInt();

			qDebug() << ps_id.toInt() << db.processedSampleName(ps_id);
		}
	}

	if(invalid_samples.size() > 0)
	{
		QMessageBox::warning(this, "Invalid samples", "The following samples were not found in the NGSD:\n" + invalid_samples.join('\n'));
	}

	return ps_ids;
}


QSet<int> BurdenTestWidget::getVariantsForRegion(int max_ngsd, double max_gnomad_af, const BedFile& regions, const QString& gene_symbol, const QStringList& impacts, bool predict_pathogenic)
{
	if(regions.count() < 1)
	{
		THROW(ArgumentException, "BED file doesn't contain any regions!");
	}

	//execute query
	QString query_text = createGeneQuery(max_ngsd, max_gnomad_af, regions, impacts, predict_pathogenic);
	SqlQuery query = db_.getQuery();
	query.exec(query_text);
	qDebug() << "initial variants:" << query.size();

	//process query
	QSet<int> variant_ids;
	int n_skipped_impact = 0;
	int n_skipped_empty = 0;
	int n_skipped_non_pathogenic = 0;
	int n_skipped_gene = 0;

	while(query.next())
	{
		//filter by impact
		QStringList parts = query.value("coding").toString().split(",");
		QStringList parts_match;
		foreach(const QString& part, parts)
		{
			//skip empty enties
			int index = part.indexOf(':');
			if (index==-1)
			{
				n_skipped_empty++;
				continue;
			}

			//skip all entries which doesn't describe the current gene
			if(!part.trimmed().startsWith(gene_symbol))
			{
				n_skipped_gene++;
				continue;
			}

			//filter by impact
			bool match = false;
			foreach(const QString& impact, impacts)
			{
				if (part.contains(impact))
				{
					double cadd = query.value("cadd").toDouble();
					double spliceai = query.value("spliceai").toDouble();
					if (predict_pathogenic && (impact != "HIGH") && (cadd < 20) && (spliceai < 0.5))
					{
						n_skipped_non_pathogenic++;
						continue;
					}
					match = true;
					break;
				}
			}
			if (match)
			{
				parts_match << part;
			}

			//TODO: filter by live-calculated impact
		}

		if (parts_match.count()==0)
		{
			n_skipped_impact++;
			continue;
		}

		// return variant id
		variant_ids.insert(query.value("id").toInt());
	}

//	qDebug() << "skipped id:" << n_skipped_id;
	qDebug() << "skipped empty:" << n_skipped_empty;
	qDebug() << "skipped wrong gene:" << n_skipped_gene;
	qDebug() << "skipped wrong impact:" << n_skipped_impact;
	qDebug() << "skipped non-pathogenic prediction: " << n_skipped_non_pathogenic;
	qDebug() << "\t -> remaining variants:" << variant_ids.size();

	return variant_ids;
}

QString BurdenTestWidget::createGeneQuery(int max_ngsd, double max_gnomad_af, const BedFile& regions, const QStringList& impacts, bool predict_pathogenic)
{
	//prepare db queries
	QString query_text = QString() + "SELECT v.* FROM variant v WHERE"
			+ " (germline_het>0 OR germline_hom>0) AND germline_het+germline_hom<=" + QString::number(max_ngsd)
			+ " AND (gnomad IS NULL OR gnomad<=" + QString::number(max_gnomad_af) + ")";
	//impacts
	if(impacts.size() > 0)
	{
		query_text += " AND (";
		QStringList impact_query_statement;
		foreach (const QString& impact, impacts)
		{
			if (!predict_pathogenic || impact == "HIGH")
			{
				impact_query_statement << "(v.coding LIKE '%" + impact + "%')";
			}
			else
			{
				impact_query_statement << "(v.coding LIKE '%" + impact + "%' AND (v.cadd>=20 OR v.spliceai>=0.5))";
			}
		}
		query_text += impact_query_statement.join(" OR ");
		query_text += ")";
	}

	//gene regions
	QStringList chr_ranges;
	for (int i = 0; i < regions.count(); ++i)
	{
		chr_ranges << "(v.chr='" + regions[i].chr().strNormalized(true) + "' AND v.start>=" + QString::number(regions[i].start()) + " AND v.end<=" + QString::number(regions[i].end()) + ")";
	}
	//collapse to final query
	query_text += " AND (" + chr_ranges.join(" OR ") + ") ORDER BY start";

	return query_text;
}

void BurdenTestWidget::performBurdenTest()
{

	if(test_running) return;
	test_running = true;
	QApplication::setOverrideCursor(Qt::BusyCursor);

	QTime timer;
	timer.start();

	//delete old table and disable sorting
	ui_->tw_gene_table->setRowCount(0);
	ui_->tw_gene_table->setSortingEnabled(false);

	//parse options
	int max_ngsd = ui_->sb_max_ngsd_count->value();
	double max_gnomad_af = ui_->sb_max_gnomad_af->value()/100.0;
	QStringList impacts;
	if (ui_->cb_high->isChecked()) impacts << "HIGH";
	if (ui_->cb_medium->isChecked()) impacts << "MODERATE";
	if (ui_->cb_low->isChecked()) impacts << "LOW";
	if (ui_->cb_modifier->isChecked()) impacts << "MODIFIER";
	bool include_mosaic = ui_->cb_include_mosaic->isChecked();
	bool predict_pathogenic = ui_->cb_predict_patogenic->isChecked();

	Inheritance inheritance = Inheritance::dominant;
	if(ui_->cb_inheritance->currentText() == "dominant (only de-novo variants)") inheritance = Inheritance::de_novo;
	if(ui_->cb_inheritance->currentText() == "recessive (only hom/compund-het variants)") inheritance = Inheritance::recessive;

	qDebug() << "get options: " << Helper::elapsedTime(timer);

	//get all processed sample ids
	QStringList ps_ids;
	foreach (int id, case_samples_ + control_samples_)
	{
		ps_ids << QString::number(id);
	}

	//prepare query
//	prepareSqlQuery(max_ngsd, max_gnomad_af, impacts, predict_pathogenic);
//	qDebug() << "prepare query: " << Helper::elapsedTime(timer);

	// get genes
	QList<int> gene_ids;
	foreach (QByteArray gene, selected_genes_)
	{
		gene_ids << db_.geneId(gene);
	}
	qDebug() << "get gene ids: " << Helper::elapsedTime(timer);


	int i=0;
	double n_sec_single_query = 0;
	// perform search
	foreach (int gene_id, gene_ids)
	{
		i++;
		QByteArray gene_name = db_.geneSymbol(gene_id);

		// get gene region
		BedFile gene_regions = db_.geneToRegions(gene_name, Transcript::ENSEMBL, "exon", true);
		gene_regions.sort();
		gene_regions.merge();

		qDebug() << gene_name << gene_regions.count();

		//skip genes without ensemble regions
		if (gene_regions.count() == 0)
		{
			qDebug() << "Gene " + gene_name + " skipped cause it has no chromosomal regions!";
			continue;
		}

		//new method
		QTime tmp_timer;
		tmp_timer.start();
		//get all variants for this gene
		QSet<int> variant_ids = getVariantsForRegion(max_ngsd, max_gnomad_af, gene_regions, gene_name, impacts, predict_pathogenic);
		n_sec_single_query += tmp_timer.elapsed();

		qDebug() << i << "get var ids for gene " + gene_name + ": "<< variant_ids.size() << Helper::elapsedTime(timer);

		qDebug() << "new method (s): " << n_sec_single_query/1000;

		// for all matching variants: get counts of case and control cohort
		QMap<int,QSet<int>> detected_variants;
		if(variant_ids.size() != 0)
		{
			QStringList var_ids_str;
			foreach (int id, variant_ids)
			{
				var_ids_str << QString::number(id);
			}
			SqlQuery detected_variant_query = db_.getQuery();
			detected_variant_query.exec("SELECT processed_sample_id, variant_id FROM detected_variant WHERE variant_id IN (" + var_ids_str.join(", ") + ") "
										+ "AND processed_sample_id IN (" + ps_ids.join(", ") + ")" + ((include_mosaic)?"":" AND mosaic=0"));
			while (detected_variant_query.next())
			{
				int ps_id = detected_variant_query.value("processed_sample_id").toInt();
				int var_id = detected_variant_query.value("variant_id").toInt();

	//			if(!detected_variants.keys().contains(ps_id)) detected_variants[ps_id] = QSet<int>();
				detected_variants[ps_id] << var_id;
			}

	//		qDebug() << "get detected variants took: " << Helper::elapsedTime(timer);
		}



		QStringList ps_names_cases;
		QStringList ps_names_controls;

		int n_cases = countOccurences(variant_ids, case_samples_, detected_variants, inheritance, ps_names_cases);
		int n_controls = countOccurences(variant_ids, control_samples_, detected_variants, inheritance, ps_names_controls);

		//sort processed samples
		std::sort(ps_names_cases.begin(), ps_names_cases.end());
		std::sort(ps_names_controls.begin(), ps_names_controls.end());

		//calculate p-value (fisher)
		double p_value = BasicStatistics::fishersExactTest(n_cases, n_controls, case_samples_.size(), control_samples_.size(), "greater");

		qDebug() << gene_name << "calculating counts took: " << Helper::elapsedTime(timer);

		//create table line
		int row_idx = ui_->tw_gene_table->rowCount();
		int column_idx = 0;
		ui_->tw_gene_table->setRowCount(row_idx+1);

		ui_->tw_gene_table->setItem(row_idx, column_idx++, GUIHelper::createTableItem(gene_name));
		ui_->tw_gene_table->setItem(row_idx, column_idx++, GUIHelper::createTableItem(p_value,  5));

		ui_->tw_gene_table->setItem(row_idx, column_idx, GUIHelper::createTableItem(n_cases));
		ui_->tw_gene_table->item(row_idx, column_idx++)->setToolTip(ps_names_cases.join(", "));
		ui_->tw_gene_table->setItem(row_idx, column_idx, GUIHelper::createTableItem(n_controls));
		ui_->tw_gene_table->item(row_idx, column_idx++)->setToolTip(ps_names_controls.join(", "));

		// add infos for hidden columns (ps names)
		ui_->tw_gene_table->setItem(row_idx, column_idx++, GUIHelper::createTableItem(ps_names_cases.join(", ")));
		ui_->tw_gene_table->setItem(row_idx, column_idx++, GUIHelper::createTableItem(ps_names_controls.join(", ")));

	}

	//final adjustments
	ui_->tw_gene_table->verticalHeader()->hide();
	ui_->tw_gene_table->setColumnHidden(4, true);
	ui_->tw_gene_table->setColumnHidden(5, true);
	GUIHelper::resizeTableCells(ui_->tw_gene_table, 200);
	ui_->tw_gene_table->setSortingEnabled(true);
	ui_->tw_gene_table->sortByColumn(1, Qt::AscendingOrder);


	qDebug() << "Burden test took: " << Helper::elapsedTime(timer);
	QApplication::restoreOverrideCursor();
	test_running = false;
}

void BurdenTestWidget::copyToClipboard()
{
	//add parameters as comments
	QStringList comments;

	//cohorts
	QStringList case_sample_list;
	foreach (int ps_id, case_samples_)
	{
		case_sample_list << db_.processedSampleName(QString::number(ps_id));
	}
	std::sort(case_sample_list.begin(), case_sample_list.end());
	comments << "cases=" + case_sample_list.join(",");

	QStringList control_sample_list;
	foreach (int ps_id, control_samples_)
	{
		control_sample_list << db_.processedSampleName(QString::number(ps_id));
	}
	std::sort(control_sample_list.begin(), control_sample_list.end());
	comments << "controls=" + control_sample_list.join(",");

	//custom gene set
	comments << "genes=" + selected_genes_.toStringList().join(",");

	//filter parameter
	comments << "max_gnomad_af=" + QString::number(ui_->sb_max_gnomad_af->value(), 'f', 2);
	comments << "max_ngsd_count=" + QString::number(ui_->sb_max_ngsd_count->value());
	QStringList impacts;
	if(ui_->cb_high->isChecked()) impacts << "HIGH";
	if(ui_->cb_medium->isChecked()) impacts << "MEDIUM";
	if(ui_->cb_low->isChecked()) impacts << "LOW";
	if(ui_->cb_modifier->isChecked()) impacts << "MODIFIER";
	comments << "impact=" + impacts.join(",");
	comments << QString("include_mosaic=") + ((ui_->cb_include_mosaic->isChecked())?"1":"0");
	comments << "inheritance=" + ui_->cb_inheritance->currentText();

	GUIHelper::copyToClipboard(ui_->tw_gene_table, false, comments);
}


QStringList BurdenTestWidget::createChromosomeQueryList(int max_ngsd, double max_gnomad_af, const BedFile& regions, const QStringList& impacts, bool predict_pathogenic, bool include_mosaic)
{
	QString ngsd_counts = (include_mosaic)?"(germline_het+germline_hom+germline_mosaic)<=":"(germline_het+germline_hom)<=";
	//prepare db queries
	QString query_text_prefix = QString() + "SELECT v.id, v.start, v.end FROM variant v WHERE"
			+ " (germline_het>0 OR germline_hom>0) AND " + ngsd_counts + QString::number(max_ngsd)
			+ " AND (gnomad IS NULL OR gnomad<=" + QString::number(max_gnomad_af) + ")"
			+ " AND v.chr='" + regions[0].chr().strNormalized(true) + "'";
	//impacts
	if(impacts.size() > 0)
	{
		query_text_prefix += " AND (";
		QStringList impact_query_statement;
		foreach (const QString& impact, impacts)
		{
			if (!predict_pathogenic || impact == "HIGH")
			{
				impact_query_statement << "(v.coding LIKE '%" + impact + "%')";
			}
			else
			{
				impact_query_statement << "(v.coding LIKE '%" + impact + "%' AND (v.cadd>=20 OR v.spliceai>=0.5))";
			}

		}
		query_text_prefix += impact_query_statement.join(" OR ");
		query_text_prefix += ")";
	}

	//gene regions
	QStringList queries;
	QStringList chr_ranges;
	for (int i = 0; i < regions.count(); ++i)
	{
		//split large query into smaller ones
		if(chr_ranges.size() >= 100)
		{
			queries << query_text_prefix + " AND " + chr_ranges.join(" OR ") + " ORDER BY start";
			chr_ranges.clear();
		}

		chr_ranges << "(v.start>=" + QString::number(regions[i].start()) + " AND v.end<=" + QString::number(regions[i].end()) + ")";
	}
	//add final batch
	queries << query_text_prefix + " AND " + chr_ranges.join(" OR ") + " ORDER BY start";

	qDebug() << chr_ranges.size();
	qDebug() << queries;
	return queries;
}

int BurdenTestWidget::countOccurences(const QSet<int>& variant_ids, const QSet<int>& ps_ids, const QMap<int, QSet<int>>& detected_variants, Inheritance inheritance, QStringList& ps_names)
{
	int n_hits = 0;
	foreach(int ps_id, ps_ids)
	{
		//check for variant in gene
		if(!detected_variants.keys().contains(ps_id)) continue;
		QSet<int> intersection = variant_ids;
		intersection = intersection.intersect(detected_variants.value(ps_id));

		if (!excluded_regions_.isEmpty())
		{
			//filter matches based on blacklist BED file
			QSet<int> filtered_intersection;
			foreach (int variant_id, intersection)
			{
				Variant var = db_.variant(QString::number(variant_id));
				if (!excluded_regions_.overlapsWith(var.chr(), var.start(), var.end())) filtered_intersection.insert(variant_id);
			}
			//update id list
			intersection = filtered_intersection;
		}

		//no match
		if (intersection.size() == 0) continue;



		//check for de-novo
		if(inheritance == Inheritance::de_novo)
		{
			int rc_id = db_.reportConfigId(QString::number(ps_id));

			// no rc_id -> no de-novo variants
			if(rc_id < 0) continue;

			//get all de-novo variants of this sample
			QSet<int> de_novo_var_ids = db_.getValuesInt("SELECT variant_id FROM report_configuration_variant WHERE de_novo=TRUE AND report_configuration_id=:0", QString::number(rc_id)).toSet();
			intersection = intersection.intersect(de_novo_var_ids);

			//no de-novo variants
			if (intersection.size() == 0) continue;
		}
		else if((inheritance == Inheritance::recessive) && (intersection.size() == 1))
		{
			// check for hom var
			QString genotype = db_.getValue("SELECT genotype FROM detected_variant WHERE processed_sample_id=" + QString::number(ps_id) + " AND variant_id="
											+ QString::number(intersection.toList().at(0))).toString();
			if (genotype != "hom") continue;
		}

		//else (at least two hits or non-ressesive)
		n_hits++;
		ps_names << db_.processedSampleName(QString::number(ps_id));
	}

	return n_hits;
}
