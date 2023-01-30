#include "BurdenTestWidget.h"
#include "ui_BurdenTestWidget.h"
#include "GlobalServiceProvider.h"
#include <GUIHelper.h>
#include <LoginManager.h>
#include <NGSD.h>
#include <QDialog>
#include <QMessageBox>
#include <QTextEdit>

BurdenTestWidget::BurdenTestWidget(QWidget *parent) :
	QWidget(parent),
	ui_(new Ui::BurdenTestWidget),
	variant_query_(db_.getQuery())
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
	connect(ui_->rb_custom, SIGNAL(toggled(bool)), this, SLOT(updateGeneSelectionMenu()));

	//prepare db queries
	QString query_text = QString("SELECT v.* FROM variant v WHERE v.chr=:0 AND v.start>=:1 AND v.end<=:2 AND (germline_het>0 OR germline_hom>0) ")
			+ "AND germline_het+germline_hom<=:3 AND (gnomad IS NULL OR gnomad<=:4) ORDER BY start";
	variant_query_.prepare(query_text);

	//initial GUI setup:
	updateGeneSelectionMenu();
	validateInputData();
	ui_->progress_bar->setVisible(false);
}

BurdenTestWidget::~BurdenTestWidget()
{
	delete ui_;
}

void BurdenTestWidget::loadCaseSamples()
{
	QSet<int> ps_ids = loadSampleList("cases", case_samples_);
	if(ps_ids.size() > 0) case_samples_ = ps_ids;

	qDebug() << "Case:" << case_samples_.size() << case_samples_;

	updateSampleCounts();
	validateInputData();
}

void BurdenTestWidget::loadControlSamples()
{
	QSet<int> ps_ids = loadSampleList("controls", control_samples_);
	if(ps_ids.size() > 0) control_samples_ = ps_ids;

	qDebug() << "Control:" << control_samples_.size() << control_samples_;

	updateSampleCounts();
	validateInputData();
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

	updateGeneCounts();
}

void BurdenTestWidget::validateInputData()
{
	QStringList errors;

//	// check processing system
//	QSet<int> processing_system_ids;
//	foreach(int ps_id, (case_samples_ + control_samples_))
//	{
//		processing_system_ids << db_.processingSystemIdFromProcessedSample(db_.processedSampleName(QString::number(ps_id)));
//	}
//	if(processing_system_ids.size() > 1)
//	{
//		QStringList processing_systems;
//		foreach (int sys_id, processing_system_ids)
//		{
//			processing_systems << db_.getProcessingSystemData(sys_id).name_short;
//		}
//		errors << "ERROR: The cohorts contain more than one processing system (" + processing_systems.join(", ") + ")! ";
//	}

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

	// check not 0
	if(case_samples_.size() < 1) errors << "ERROR: The case cohort doesn't contain any samples!";
	if(control_samples_.size() < 1) errors << "ERROR: The control cohort doesn't contain any samples!";


	//check custom gene set
	if(ui_->rb_custom->isChecked() && selected_genes_.count() < 1) errors << "ERROR: Custom gene mode, but no genes selected!";

	if(errors.size() > 0)
	{
		QMessageBox::warning(this, "Validation error", "During cohort validation the following errors occured:\n" +  errors.join("\n"));
		ui_->b_burden_test->setEnabled(false);
	}
	else
	{
		ui_->b_burden_test->setEnabled(true);
	}

	qDebug() << "Case:" << case_samples_.size() << case_samples_;
	qDebug() << "Control:" << control_samples_.size() << control_samples_;
}

void BurdenTestWidget::updateSampleCounts()
{
	ui_->l_cases->setText("(" + QString::number(case_samples_.size()) + " Samples)");
	ui_->l_controls->setText("(" + QString::number(control_samples_.size()) + " Samples)");
}

void BurdenTestWidget::updateGeneCounts()
{
	ui_->l_gene_count->setText("(" + QString::number(selected_genes_.count()) + " genes)");
	ui_->l_genes->setText(selected_genes_.toStringList().join(", "));
}

void BurdenTestWidget::updateGeneSelectionMenu()
{
	ui_->b_load_genes->setEnabled(ui_->rb_custom->isChecked());
	ui_->l_gene_count->setEnabled(ui_->rb_custom->isChecked());
	ui_->l_genes->setEnabled(ui_->rb_custom->isChecked());
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

QSet<int> BurdenTestWidget::getVariantsForRegion(const BedLine& region, const QString& gene_symbol, const QStringList& impacts, const QSet<int>& valid_variant_ids)
{
	QSet<int> variant_ids;

	//bind values and execute query
	variant_query_.bindValue(0, region.chr().strNormalized(true));
	variant_query_.bindValue(1, region.start());
	variant_query_.bindValue(2, region.end());
	variant_query_.exec();

	qDebug() << "SQL results for gene " +  gene_symbol + ": " << variant_query_.size();
	int n_skipped_id = 0;
	int n_skipped_impact = 0;
	int n_skipped_empty = 0;
	int n_skipped_gene = 0;


	//get variants in chromosomal range
//	QSet<QString> vars_distinct;
//	QList<QList<QVariant>> var_data;
	while(variant_query_.next())
	{
		int var_id = variant_query_.value("id").toInt();
		//skip variants which doesn't occure in cohort
		if((valid_variant_ids.size() > 0) && !valid_variant_ids.contains(var_id))
		{
			n_skipped_id++;
			continue;
		}

//		QString var = variant_query_.value("chr").toString() + ":" + variant_query_.value("start").toString() + "-" + variant_query_.value("end").toString() + " "
//				+ variant_query_.value("ref").toString() + " > " + variant_query_.value("obs").toString();
//		QVariant gnomad = variant_query_.value("gnomad");
//		QVariant cadd = variant_query_.value("cadd");
//		QVariant spliceai = variant_query_.value("spliceai");

		//filter by impact
		QStringList parts = variant_query_.value("coding").toString().split(",");
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
			if(!part.trimmed().startsWith(gene_symbol)) continue;

			//filter by impact
			bool match = false;
			foreach(const QString& impact, impacts)
			{
				if (part.contains(impact))
				{
					match = true;
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

//		int germline_het = variant_query_.value("germline_het").toInt();
//		int germline_hom = variant_query_.value("germline_hom").toInt();

		// return variant id
		variant_ids << variant_query_.value("id").toInt();

	}

	qDebug() << "skipped id:" << n_skipped_id;
	qDebug() << "skipped wrong gene:" << n_skipped_gene;
	qDebug() << "skipped wrong impact:" << n_skipped_impact;

	return variant_ids;

}

void BurdenTestWidget::performBurdenTest()
{
	if(test_running) return;
	test_running = true;
	ui_->progress_bar->setVisible(true);
	ui_->progress_bar->setValue(0);

	QTime timer;
	timer.start();

	// get ids all detected variants
	QSet<int> all_var_ids;
//	int n = 0;
//	foreach(int ps_id, case_samples_)
//	{
//		n++;
//		QList<int> var_ids = db_.getValuesInt("SELECT variant_id FROM detected_variant WHERE processed_sample_id=:0 AND mosaic=0", QString::number(ps_id));
//		all_var_ids +=  var_ids.toSet();
//		qDebug() << n << all_var_ids.size();
//	}
//	foreach(int ps_id, control_samples_)
//	{
//		n++;
//		QList<int> var_ids = db_.getValuesInt("SELECT variant_id FROM detected_variant WHERE processed_sample_id=:0 AND mosaic=0", QString::number(ps_id));
//		all_var_ids +=  var_ids.toSet();
//		qDebug() << n << all_var_ids.size();
//	}

//	qDebug() << "get cohort var ids: " << Helper::elapsedTime(timer);

//	qDebug() << "var ids:" << all_var_ids;


	//parse options
	int max_ngsd = ui_->sb_max_ngsd_count->value();
	double max_gnomad_af = ui_->sb_max_gnomad_af->value()/100.0;
	QStringList impacts;
	if (ui_->cb_high->isChecked()) impacts << "HIGH";
	if (ui_->cb_medium->isChecked()) impacts << "MODERATE";
	if (ui_->cb_low->isChecked()) impacts << "LOW";
	if (ui_->cb_modifier->isChecked()) impacts << "MODIFIER";
	bool include_mosaic = ui_->cb_include_mosaic->isChecked();
	bool recessive = ui_->cb_recessive->isChecked();

	qDebug() << "get options: " << Helper::elapsedTime(timer);

	//get all processed sample ids
	QStringList ps_ids;
	foreach (int id, case_samples_ + control_samples_)
	{
		ps_ids << QString::number(id);
	}

	//prepare query
	prepareSqlQuery(max_ngsd, max_gnomad_af, ps_ids, impacts);

	qDebug() << "prepare query: " << Helper::elapsedTime(timer);

	// get genes
	QList<int> gene_ids;
	if(ui_->rb_custom->isChecked())
	{
		foreach (QByteArray gene, selected_genes_)
		{
			gene_ids << db_.geneId(gene);
		}
	}
	else
	{
		gene_ids = db_.getValuesInt(QString() + "SELECT `id` FROM `gene`" + ((ui_->rb_protein_coding->isChecked())? " WHERE `type`='protein-coding gene'": ""));
	}
	qDebug() << "get gene ids: " << Helper::elapsedTime(timer);





//	//test
//	//prepare db queries
//	SqlQuery test_query = db_.getQuery();
//	QStringList ps_ids;
//	qDebug() << case_samples_.size() << case_samples_;
//	qDebug() << control_samples_.size() << control_samples_;
//	qDebug() << (case_samples_ + control_samples_).size() << case_samples_ + control_samples_;

//	foreach (int i, case_samples_ + control_samples_)
//	{
//		ps_ids << QString::number(i);
//	}
//	QString query_text = "SELECT v.id, v.coding FROM variant v INNER JOIN detected_variant dv ON v.id=dv.variant_id WHERE v.chr=:0 AND (germline_het>0 OR germline_hom>0) AND germline_het+germline_hom<=:1 AND (gnomad IS NULL OR gnomad<=:2) AND dv.processed_sample_id IN (" + ps_ids.join(", ") + ") AND dv.mosaic=0 ORDER BY start";
//	qDebug() << query_text;

//	test_query.prepare(query_text);

//	test_query.bindValue(0, "chr5");
//	test_query.bindValue(1, max_ngsd);
//	test_query.bindValue(2, max_gnomad_af);

//	test_query.exec();

//	qDebug() << "Query for whole chr5 took: " << Helper::elapsedTime(timer);
//	qDebug() << "Query size: " << test_query.size();

//	int i=0;
//	while(test_query.next())
//	{
//		qDebug() << i << test_query.value("id") ; //<< test_query.value("coding");
//		i++;
//	}

//	qDebug() << "Query for whole chr5 took: " << Helper::elapsedTime(timer);
//	qDebug() << "Query size: " << test_query.size();



	int i=0;
	// perform search
	foreach (int gene_id, gene_ids)
	{
		i++;
		QByteArray gene_name = db_.geneSymbol(gene_id);

		// get gene region
		BedFile gene_regions = db_.geneToRegions(gene_name, Transcript::ENSEMBL, "gene");
		gene_regions.sort();
		gene_regions.merge();

		qDebug() << gene_name << gene_regions.count();

		//skip genes without ensemble regions
		if (gene_regions.count() == 0) continue;

		qDebug() << gene_regions[0].toString(true);

		//get all variants for this gene
		QSet<int> variant_ids;
		for (int j = 0; j < gene_regions.count(); ++j)
		{
			variant_ids += getVariantsForRegion(gene_regions[j], gene_name, impacts, all_var_ids);
			qDebug() << variant_ids.size();
		}

		qDebug() << i << "get var ids for gene " + gene_name + ": "<< variant_ids.size() << Helper::elapsedTime(timer);

		//skip genes with no selected variants
		if(variant_ids.size() == 0) continue;

		// for all matching variants: get counts of case and control cohort
		QMap<int,QSet<int>> detected_variants;
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

			if(!detected_variants.keys().contains(ps_id)) detected_variants[ps_id] = QSet<int>();
			detected_variants[ps_id] << var_id;
		}

		qDebug() << "get detected variants took: " << Helper::elapsedTime(timer);

		int n_cases = 0;
		int n_controls = 0;
		QStringList ps_names_cases;
		QStringList ps_names_controls;

		foreach(int ps_id, case_samples_)
		{
			//check for variant in gene
			if(!detected_variants.keys().contains(ps_id)) continue;
			QSet<int> intersection = variant_ids.intersect(detected_variants.value(ps_id));

			//no match
			if (intersection.size() == 0) continue;

			if(recessive && (intersection.size() == 1))
			{
				// check for hom var
				QString genotype = db_.getValue("SELECT genotype FROM detected_variant WHERE processed_sample_id=" + QString::number(ps_id) + " AND variant_id="
												+ QString::number(intersection.toList().at(0))).toString();

				qDebug() << genotype;
				if (genotype != "hom") continue;
			}

			//else (at least two hits or non-ressesive)
			n_cases++;
			ps_names_cases << db_.processedSampleName(QString::number(ps_id));
		}

		foreach(int ps_id, control_samples_)
		{
			//check for variant in gene
			if(!detected_variants.keys().contains(ps_id)) continue;
			QSet<int> intersection = variant_ids.intersect(detected_variants.value(ps_id));

			//no match
			if (intersection.size() == 0) continue;

			if(recessive && (intersection.size() == 1))
			{
				// check for hom var
				QString genotype = db_.getValue("SELECT genotype FROM detected_variant WHERE processed_sample_id=" + QString::number(ps_id) + " AND variant_id="
												+ QString::number(intersection.toList().at(0))).toString();

				qDebug() << genotype;
				if (genotype != "hom") continue;
			}

			//else (at least two hits or non-ressesive)
			n_controls++;
			ps_names_controls << db_.processedSampleName(QString::number(ps_id));

		}
		qDebug() << "calculating counts took: " << Helper::elapsedTime(timer);

		//calculate p-value (fisher)
		double p_value = BasicStatistics::fishersExactTest(n_cases, n_controls, case_samples_.size(), control_samples_.size());

		//create table line
		int row_idx = ui_->tw_gene_table->rowCount();
		int column_idx = 0;
		ui_->tw_gene_table->setRowCount(row_idx+1);

		ui_->tw_gene_table->setItem(row_idx, column_idx++, GUIHelper::createTableItem(gene_name));
		ui_->tw_gene_table->setItem(row_idx, column_idx++, GUIHelper::createTableItem(QString::number(p_value, 'f', 5)));

		ui_->tw_gene_table->setItem(row_idx, column_idx, GUIHelper::createTableItem(QString::number(n_cases)));
		ui_->tw_gene_table->item(row_idx, column_idx++)->setToolTip(ps_names_cases.join(", "));
		ui_->tw_gene_table->setItem(row_idx, column_idx, GUIHelper::createTableItem(QString::number(n_controls)));
		ui_->tw_gene_table->item(row_idx, column_idx++)->setToolTip(ps_names_controls.join(", "));

		//update progress bar
		ui_->progress_bar->setValue(int(((float)i/gene_ids.size())*100));
		this->update();

	}

	GUIHelper::resizeTableCells(ui_->tw_gene_table, 200);


	qDebug() << "Burden test took: " << Helper::elapsedTime(timer);
	test_running = false;
	ui_->progress_bar->setVisible(false);
}

void BurdenTestWidget::prepareSqlQuery(int max_ngsd, double max_gnomad_af, const QStringList& ps_ids, const QStringList& impacts)
{
	//prepare db queries
	QString query_text = QString() + "SELECT v.* FROM variant v WHERE"
			+ " (germline_het>0 OR germline_hom>0) AND germline_het+germline_hom<=" + QString::number(max_ngsd)
			+ " AND (gnomad IS NULL OR gnomad<=" + QString::number(max_gnomad_af) + ")"
			+ " AND v.chr=:0 AND v.start>=:1 AND v.end<=:2";
//	if(impacts.size() > 0)
//	{
//		query_text += " AND (";
//		QStringList impact_query_statement;
//		foreach (const QString& impact, impacts)
//		{
//			impact_query_statement << "v.coding LIKE '%" + impact + "%'";
//		}
//		query_text += impact_query_statement.join(" OR ");
//		query_text += ")";
//	}
	query_text += " ORDER BY start";

	variant_query_.prepare(query_text);
}
