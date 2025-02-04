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
#include "GSvarHelper.h"


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
	connect(ui_->cb_inheritance, SIGNAL(currentIndexChanged(int)), this, SLOT(updateCNVCheckbox()));
	connect(ui_->cb_only_ccr, SIGNAL(stateChanged(int)), this, SLOT(validateCCRGenes()));

	//initial GUI setup:
	//disable test start until data is validated
	ui_->b_burden_test->setEnabled(false);

	initCCR();

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

	qDebug() << "Case:" << case_samples_.size();

	updateSampleCounts();
	//disable test start until data is validated
	ui_->b_burden_test->setEnabled(false);
}

void BurdenTestWidget::loadControlSamples()
{
	QSet<int> ps_ids = loadSampleList("controls", control_samples_);
	if(ps_ids.size() > 0) control_samples_ = ps_ids;
	controls_initialized_ = (control_samples_.size() > 0);

	qDebug() << "Control:" << control_samples_.size();

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
	GeneSet selected_genes;
	QByteArray input_text = te_genes->toPlainText().toUtf8();
	if(input_text.contains('\t'))
	{
		//TSV
		foreach (QByteArray line, input_text.split('\n'))
		{
			if(line.startsWith("#")) continue;
			selected_genes.insert(line.split('\t').at(0));
		}
	}
	else if(input_text.contains(';') || input_text.contains(','))
	{
		//comma/semicolon separated list
		selected_genes = GeneSet::createFromText(input_text.replace(",", ";"));
	}
	else
	{
		//one line per gene
		selected_genes = GeneSet::createFromText(te_genes->toPlainText().toUtf8());
	}


	selected_genes = db_.genesToApproved(selected_genes, true);
	GeneSet approved_genes = db_.genesToApproved(selected_genes, false);

	QSet<QByteArray> invalid_genes = selected_genes.toSet() - approved_genes.toSet();

	if (invalid_genes.size() > 0)
	{
		QMessageBox::warning(this, "Invalid genes", "The following genes couldn't be converted into approved gene names and were removed:\n" + invalid_genes.toList().join(", "));
	}

	//apply new gene set
	selected_genes_ = approved_genes;

	gene_set_initialized_ = true;

	updateGeneCounts();
	validateCCRGenes();
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

		//check for file name
		QString bed_file_path = te_excluded_regions_->property("file_path").toString();
		if (!bed_file_path.trimmed().isEmpty()) excluded_regions_file_names << bed_file_path.trimmed();

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
	excluded_regions_file_names.clear();
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
		te_excluded_regions_->setProperty("file_path", bed_file_path);
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

	//table for relation warnings
	QStringList warning_table_header;
	warning_table_header << "Sample" << "Cohort" << "Problem" << "Affected samples" << "Keep";
	QList<QStringList> warning_table;

	// check overlap (processed samples)
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
	QSet<QString> processing_systems;
	QSet<QString> ancestry;
	QMap<int,QList<int>> sample_ids_cases;
	QMap<int,QList<int>> sample_ids_controls;

	//get all sample ids for the given processed samples
	foreach (int ps_id, case_samples_)
	{
		QString ps_name = db_.processedSampleName(QString::number(ps_id));
		ProcessedSampleData ps_info = db_.getProcessedSampleData(QString::number(ps_id));
		processing_systems.insert(ps_info.processing_system);
		ancestry.insert(ps_info.ancestry);

		sample_ids_cases[db_.sampleId(ps_name).toInt()].append(ps_id);
	}
	foreach (int ps_id, control_samples_)
	{
		QString ps_name = db_.processedSampleName(QString::number(ps_id));
		ProcessedSampleData ps_info = db_.getProcessedSampleData(QString::number(ps_id));
		processing_systems.insert(ps_info.processing_system);
		ancestry.insert(ps_info.ancestry);

		sample_ids_controls[db_.sampleId(ps_name).toInt()].append(ps_id);
	}

	// check overlap (samples)
	overlap = sample_ids_cases.keys().toSet() & sample_ids_controls.keys().toSet();
	if(overlap.size() > 0)
	{
		QStringList sample_names;
		foreach (int s_id, overlap)
		{
			sample_names << db_.sampleName(QString::number(s_id));
		}
		errors << "ERROR: The samples " + sample_names.join(", ") + " are present in both case and control cohort!";
	}

	//abort, no need to check relations
	if(errors.size() > 0)
	{
		QMessageBox::warning(this, "Validation error", "During cohort validation the following errors occured:\n" +  errors.join("\n"));
		ui_->b_burden_test->setEnabled(false);
		return;
	}

	//check for duplicate samples in cases/controls
	foreach (int s_id, sample_ids_cases.keys())
	{
		if(sample_ids_cases.value(s_id).size() > 1)
		{
			QSet<int> ps_ids = sample_ids_cases.value(s_id).toSet();
			QStringList ps_list;
			foreach (int id, ps_ids) ps_list << db_.processedSampleName(QString::number(id));
			int ps_id = getNewestProcessedSample(ps_ids);
			ps_ids.remove(ps_id);
			foreach (int ps_id_to_remove, ps_ids)
			{
				sample_ids_cases[s_id].removeAll(ps_id_to_remove);
			}
			//sanity check
			if(sample_ids_cases.value(s_id).size() > 1) THROW(ProgrammingException, "List still contains multiple values. This should not happen!")
			warning_table.append(QStringList() << db_.sampleName(QString::number(s_id)) << "cases" << "multiple processed samples" << ps_list.join(", ") << db_.processedSampleName(QString::number(ps_id)));
		}
	}
	foreach (int s_id, sample_ids_controls.keys())
	{
		if(sample_ids_controls.value(s_id).size() > 1)
		{
			QSet<int> ps_ids = sample_ids_controls.value(s_id).toSet();
			QStringList ps_list;
			foreach (int id, ps_ids) ps_list << db_.processedSampleName(QString::number(id));
			int ps_id = getNewestProcessedSample(ps_ids);
			ps_ids.remove(ps_id);
			foreach (int ps_id_to_remove, ps_ids)
			{
				sample_ids_controls[s_id].removeAll(ps_id_to_remove);
			}
			//sanity check
			if(sample_ids_controls.value(s_id).size() > 1) THROW(ProgrammingException, "List still contains multiple values. This should not happen!")
			warning_table.append(QStringList() << db_.sampleName(QString::number(s_id)) << "controls" << "multiple processed samples" << ps_list.join(", ") << db_.processedSampleName(QString::number(ps_id)));
		}
	}

	//check for same-sample relations
	//cases
	QSet<int> s_ids_to_remove_cases;
	QSet<int> s_ids_to_remove_controls;
	foreach (int s_id, sample_ids_cases.keys())
	{
		//skip samples which will be removed anyways
		if(s_ids_to_remove_cases.contains(s_id)) continue;
		QSet<int> same_samples = db_.sameSamples(s_id, SameSampleMode::SAME_PATIENT);
		//add sample itself
		same_samples.insert(s_id);
		QSet<int> same_sample_overlap = same_samples & sample_ids_cases.keys().toSet();
		if (same_sample_overlap.size() > 1)
		{
			QStringList same_sample_list;
			foreach (int id, same_sample_overlap)
			{
				same_sample_list << db_.sampleName(QString::number(id));
			}
			int newest_s_id = getNewestSample(same_sample_overlap);
			same_sample_overlap.remove(newest_s_id);
			s_ids_to_remove_cases += same_sample_overlap;
			warning_table.append(QStringList() << db_.sampleName(QString::number(s_id)) << "cases" << "same-sample/same-patient relation" << same_sample_list.join(", ") << db_.sampleName(QString::number(newest_s_id)));
		}
		//check controls and remove all overlaps
		same_sample_overlap = same_samples & sample_ids_controls.keys().toSet();
		if (same_sample_overlap.size() > 0)
		{
			QStringList same_sample_list;
			// add case sample to warning
			same_sample_list <<  db_.sampleName(QString::number(s_id));
			foreach (int id, same_sample_overlap)
			{
				same_sample_list << db_.sampleName(QString::number(id));
			}
			s_ids_to_remove_controls += same_sample_overlap;
			warning_table.append(QStringList() << db_.sampleName(QString::number(s_id)) << "cases/controls" << "same-sample/same-patient relation" << same_sample_list.join(", ") << db_.sampleName(QString::number(s_id)));
		}
	}
	//remove samples
	foreach (int s_id, s_ids_to_remove_cases)
	{
		sample_ids_cases.remove(s_id);
	}
	foreach (int s_id, s_ids_to_remove_controls)
	{
		sample_ids_controls.remove(s_id);
	}
	s_ids_to_remove_cases.clear();
	s_ids_to_remove_controls.clear();

	//controls
	foreach (int s_id, sample_ids_controls.keys())
	{
		//skip samples which will be removed anyways
		if(s_ids_to_remove_controls.contains(s_id)) continue;
		QSet<int> same_samples = db_.sameSamples(s_id, SameSampleMode::SAME_PATIENT);
		//add sample itself
		same_samples.insert(s_id);
		QSet<int> same_sample_overlap = same_samples & sample_ids_controls.keys().toSet();
		if (same_sample_overlap.size() > 1)
		{
			//add sample itself
			same_sample_overlap.insert(s_id);
			QStringList same_sample_list;
			foreach (int id, same_sample_overlap)
			{
				same_sample_list << db_.sampleName(QString::number(id));
			}
			int newest_s_id = getNewestSample(same_sample_overlap);
			same_sample_overlap.remove(newest_s_id);
			s_ids_to_remove_controls += same_sample_overlap;
			warning_table.append(QStringList() << db_.sampleName(QString::number(s_id)) << "controls" << "same-sample/same-patient relation" << same_sample_list.join(", ") << db_.sampleName(QString::number(newest_s_id)));
		}

	}
	foreach (int s_id, s_ids_to_remove_controls)
	{
		sample_ids_controls.remove(s_id);
	}
	s_ids_to_remove_controls.clear();

	//check for related samples
	QStringList relation_types = db_.getEnum("sample_relations", "relation");
	//cases
	foreach (int s_id, sample_ids_cases.keys())
	{
		//skip samples which will be removed anyways
		if(s_ids_to_remove_cases.contains(s_id)) continue;
		QStringList stati_to_remove;
		stati_to_remove << "Unaffected" << "n/a";
		foreach (const QString& relation, relation_types)
		{
			QSet<int> related_sample_overlap = db_.relatedSamples(s_id, relation) &  sample_ids_cases.keys().toSet();
			if (related_sample_overlap.size() > 1)
			{
				QStringList related_sample_list;
				foreach (int id, related_sample_overlap)
				{
					related_sample_list << db_.sampleName(QString::number(id));
				}
				QSet<int> affected_samples = removeDiseaseStatus(related_sample_overlap, stati_to_remove);
				int newest_s_id;
				if(affected_samples.size() > 0)
				{
					newest_s_id = getNewestSample(affected_samples);
				}
				else
				{
					//if no affected sample: -> use the newest
					newest_s_id = getNewestSample(related_sample_overlap);
				}
				related_sample_overlap.remove(newest_s_id);
				s_ids_to_remove_cases += related_sample_overlap;

				warning_table.append(QStringList() << db_.sampleName(QString::number(s_id)) << "cases" << relation + " relation" << related_sample_list.join(", ") << db_.sampleName(QString::number(newest_s_id)));

			}

			//check/remove related samples from controls
			related_sample_overlap = db_.relatedSamples(s_id, relation) &  sample_ids_controls.keys().toSet();
			if (related_sample_overlap.size() > 1)
			{
				QStringList related_sample_list;
				foreach (int id, related_sample_overlap)
				{
					related_sample_list << db_.sampleName(QString::number(id));
				}
				s_ids_to_remove_controls += related_sample_overlap;

				warning_table.append(QStringList() << db_.sampleName(QString::number(s_id)) << "cases/controls" << relation + " relation" << related_sample_list.join(", ") << db_.sampleName(QString::number(s_id)));
			}
		}
	}
	//remove samples
	foreach (int s_id, s_ids_to_remove_cases)
	{
		sample_ids_cases.remove(s_id);
	}
	foreach (int s_id, s_ids_to_remove_controls)
	{
		sample_ids_controls.remove(s_id);
	}
	s_ids_to_remove_cases.clear();
	s_ids_to_remove_controls.clear();


	//controls
	foreach (int s_id, sample_ids_controls.keys())
	{
		//skip samples which will be removed anyways
		if(s_ids_to_remove_controls.contains(s_id)) continue;
		QStringList stati_to_remove;
		stati_to_remove << "Affected";
		foreach (const QString& relation, relation_types)
		{
			QSet<int> related_sample_overlap = db_.relatedSamples(s_id, relation) &  sample_ids_controls.keys().toSet();
			if (related_sample_overlap.size() > 1)
			{
				QStringList related_sample_list;
				foreach (int id, related_sample_overlap)
				{
					related_sample_list << db_.sampleName(QString::number(id));
				}
				QSet<int> unaffected_samples = removeDiseaseStatus(related_sample_overlap, stati_to_remove);
				int newest_s_id;
				if(unaffected_samples.size() >0)
				{
					newest_s_id = getNewestSample(unaffected_samples);
				}
				else
				{
					//if no affected sample: -> use the newest
					newest_s_id = getNewestSample(related_sample_overlap);
				}
				related_sample_overlap.remove(newest_s_id);
				s_ids_to_remove_controls += related_sample_overlap;

				warning_table.append(QStringList() << db_.sampleName(QString::number(s_id)) << "controls" << relation + " relation" << related_sample_list.join(", ") << db_.sampleName(QString::number(s_id)));
			}
		}
	}
	//remove samples
	foreach (int s_id, s_ids_to_remove_controls)
	{
		sample_ids_controls.remove(s_id);
	}
	s_ids_to_remove_controls.clear();



	if (processing_systems.size() > 1) warnings << "WARNING: The cohort contains multiple processing systems (" + processing_systems.toList().join(", ")+ ")!";
	if (processing_systems.size() > 1) warnings << "WARNING: The cohort contains multiple ancestries (" + ancestry.toList().join(", ")+ ")!";


	else
	{
		// enable if control and case samples are set
		ui_->b_burden_test->setEnabled(cases_initialized_ && controls_initialized_ && (selected_genes_.count() > 0));
	}

	if((warnings.size() > 0 )|| (warning_table.size() > 0))
	{
		//create dialog
		tw_warnings_ = new QTableWidget();
		int n_rows = warning_table.size();
		int n_cols = warning_table_header.size();
		tw_warnings_->setRowCount(n_rows);
		tw_warnings_->setColumnCount(n_cols);
		for (int col = 0; col < n_cols; ++col)
		{
			//set header
			tw_warnings_->setHorizontalHeaderItem(col, GUIHelper::createTableItem(warning_table_header.at(col)));
			for (int row = 0; row < n_rows; ++row)
			{
				tw_warnings_->setItem(row, col, GUIHelper::createTableItem(warning_table.at(row).at(col)));
			}
		}
		GUIHelper::resizeTableCellWidths(tw_warnings_);
		GUIHelper::resizeTableCellHeightsToFirst(tw_warnings_);

		//create table view with copy option
		QWidget* view = new QWidget();
		QVBoxLayout* v_layout = new QVBoxLayout(view);
		v_layout->addWidget(new QLabel(warnings.join("<br>")));
		v_layout->setSpacing(3);
		v_layout->setMargin(0);
		if(warning_table.size() > 0)
		{
			QPushButton* bt_copy_table = new QPushButton(QIcon(":/Icons/CopyClipboard.png"), "");
			bt_copy_table->setFixedWidth(25);
			connect(bt_copy_table, SIGNAL(clicked(bool)), this, SLOT(copyWarningsToClipboard()));
			QWidget* h_view = new QWidget();
			QVBoxLayout* h_layout = new QVBoxLayout(h_view);
			h_layout->addWidget(bt_copy_table);
			h_layout->addSpacerItem(new QSpacerItem(1,1, QSizePolicy::Expanding, QSizePolicy::Fixed));
			h_layout->setMargin(0);
			v_layout->addWidget(tw_warnings_);
			v_layout->addWidget(h_view);
		}

		QSharedPointer<QDialog> dialog  = GUIHelper::createDialog(view, "Validation warning", "During cohort validation the following problems occured:", true);
		dialog->setMinimumWidth(800);
		dialog->setMaximumHeight(600);

		//show dialog
		dialog->exec();
	}

	//automatically remove samples
	//get all remaining processed samples
	QSet<int> case_samples_to_keep;
	QSet<int> control_samples_to_keep;
	foreach(int s_id, sample_ids_cases.keys())
	{
		case_samples_to_keep += sample_ids_cases.value(s_id).toSet();
	}
	foreach(int s_id, sample_ids_controls.keys())
	{
		control_samples_to_keep += sample_ids_controls.value(s_id).toSet();
	}

	QSet<int> case_samples_to_remove = case_samples_.subtract(case_samples_to_keep);
	QSet<int> control_samples_to_remove = control_samples_.subtract(control_samples_to_keep);

	if((case_samples_to_remove.size() > 0) || (control_samples_to_remove.size() > 0))
	{
		QStringList case_ps_to_remove;
		foreach (int ps_id, case_samples_to_remove)
		{
			case_ps_to_remove << db_.processedSampleName(QString::number(ps_id));
		}
		std::sort(case_ps_to_remove.begin(), case_ps_to_remove.end());
		QStringList control_ps_to_remove;
		foreach (int ps_id, control_samples_to_remove)
		{
			control_ps_to_remove << db_.processedSampleName(QString::number(ps_id));
		}
		std::sort(control_ps_to_remove.begin(), control_ps_to_remove.end());

		QLabel* label = new QLabel(this);
		label->setText(((case_ps_to_remove.size()>0)?"Cases:\n" + case_ps_to_remove.join(", ") + "\n\n":"")
					   + ((control_ps_to_remove.size()>0)?"Controls:\n" + control_ps_to_remove.join(", ") + "\n\n":"")
					   + "Would you like to procreed?");
		label->setWordWrap(true);
		auto dlg = GUIHelper::createDialog(label, "Removing samples", "The following related samples can be removed automatically:\n ", true);
		int btn = dlg->exec();
		if (btn == 1)
		{
			case_samples_ = case_samples_to_keep;
			control_samples_ = control_samples_to_keep;
			updateSampleCounts();
		}
	}
	else
	{
		// keep all samples
		case_samples_ = case_samples_to_keep;
		control_samples_ = control_samples_to_keep;
	}

	qDebug() << "Case:" << case_samples_.size();
	qDebug() << "Control:" << control_samples_.size();

	//activate Burden test
	ui_->b_burden_test->setEnabled(true);

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

			//TODO: filter by live-calculated impact?
		}

		if (parts_match.count()==0)
		{
			n_skipped_impact++;
			continue;
		}

		// return variant id
		variant_ids.insert(query.value("id").toInt());
	}

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
		chr_ranges << "(v.chr='" + regions[i].chr().strNormalized(true) + "' AND v.end>=" + QString::number(regions[i].start()) + " AND v.start<=" + QString::number(regions[i].end()) + ")";
	}
	//collapse to final query
	query_text += " AND (" + chr_ranges.join(" OR ") + ") ORDER BY start";

	return query_text;
}

int BurdenTestWidget::getNewestProcessedSample(const QSet<int>& ps_list)
{
	QMap<int,int> sample_map;
	foreach (int ps_id, ps_list)
	{
		int process_id = db_.getValue("SELECT `process_id` FROM `processed_sample` WHERE `id`=:0", false, QString::number(ps_id)).toInt();
		sample_map.insert(process_id, ps_id);
	}
	//since QMap is sorted last element ist newest
	return sample_map.last();
}

int BurdenTestWidget::getNewestSample(const QSet<int>& s_list)
{
	QMap<QDate,int> sample_map;
	foreach (int s_id, s_list)
	{
		QDate received_date = db_.getValue("SELECT `received` FROM `sample` WHERE `id`=:0", false, QString::number(s_id)).toDate();
		sample_map.insert(received_date, s_id);
	}
	//since QMap is sorted last element ist newest
	return sample_map.last();
}

QSet<int> BurdenTestWidget::removeDiseaseStatus(const QSet<int>& s_list, const QStringList& stati_to_remove)
{
	QSet<int> s_ids;
	foreach (int s_id, s_list)
	{
		QString disease_status = db_.getValue("SELECT `disease_status` FROM `sample` WHERE `id`=:0", false, QString::number(s_id)).toString();
		if(!stati_to_remove.contains(disease_status))
		{
			s_ids << s_id;
		}
	}
	return s_ids;
}

void BurdenTestWidget::performBurdenTest()
{

	if(test_running_) return;
	test_running_ = true;
	QApplication::setOverrideCursor(Qt::BusyCursor);

	bool include_cnvs = ui_->cb_include_cnvs->isChecked();

	QTime timer;
	timer.start();

	//delete old table and disable sorting
	ui_->tw_gene_table->setRowCount(0);
	ui_->tw_gene_table->setSortingEnabled(false);
	ui_->tw_gene_table->setColumnCount(6);
	ui_->tw_gene_table->setHorizontalHeaderItem(0, GUIHelper::createTableItem("gene"));
	ui_->tw_gene_table->setHorizontalHeaderItem(1, GUIHelper::createTableItem("p-value"));
	ui_->tw_gene_table->setHorizontalHeaderItem(2, GUIHelper::createTableItem("counts cases"));
	ui_->tw_gene_table->setHorizontalHeaderItem(3, GUIHelper::createTableItem("counts controls"));
	ui_->tw_gene_table->setHorizontalHeaderItem(4, GUIHelper::createTableItem("samples cases"));
	ui_->tw_gene_table->setHorizontalHeaderItem(5, GUIHelper::createTableItem("samples controls"));
	if(include_cnvs)
	{
		ui_->tw_gene_table->setColumnCount(10);
		ui_->tw_gene_table->setHorizontalHeaderItem(6, GUIHelper::createTableItem("counts cases (CNVs)"));
		ui_->tw_gene_table->setHorizontalHeaderItem(7, GUIHelper::createTableItem("counts controls (CNVs)"));
		ui_->tw_gene_table->setHorizontalHeaderItem(8, GUIHelper::createTableItem("samples cases (CNVs)"));
		ui_->tw_gene_table->setHorizontalHeaderItem(9, GUIHelper::createTableItem("samples controls (CNVs)"));
	}

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

	//Debug:
	qDebug() << "case samples: " << case_samples_.size();
	qDebug() << "control samples: " << control_samples_.size();
	qDebug() << "combined list: " << ps_ids.size();

	// get genes
	QList<int> gene_ids;
	foreach (QByteArray gene, selected_genes_)
	{
		gene_ids << db_.geneId(gene);
	}
	qDebug() << "get gene ids: " << Helper::elapsedTime(timer);

	QSet<int> callset_ids_cases;
	QSet<int> callset_ids_controls;
	BedFile cnv_polymorphism_region;
	if (include_cnvs)
	{
		//get callset ids for each processed sample
		SqlQuery cnv_callset_query = db_.getQuery();
		cnv_callset_query.prepare("SELECT id, quality_metrics FROM cnv_callset WHERE processed_sample_id=:0");
		foreach (int ps_id, case_samples_)
		{
			//get sample type
			QString processing_system_type = db_.getProcessedSampleData(QString::number(ps_id)).processing_system_type;
			double min_correlation = (processing_system_type == "WGS")? 0.55: 0.9;
			cnv_callset_query.bindValue(0, ps_id);
			cnv_callset_query.exec();
			while(cnv_callset_query.next())
			{
				int callset_id = cnv_callset_query.value(0).toInt();
				QMap<QString, QVariant> quality_metrics = QJsonDocument::fromJson(cnv_callset_query.value(1).toByteArray()).object().toVariantMap();
				double ref_correlation = quality_metrics.value("mean correlation to reference samples").toDouble();
				qDebug() << quality_metrics;
				qDebug() << ref_correlation;
				if (ref_correlation >= min_correlation) callset_ids_cases << callset_id;
			}
			qDebug() << "callset ids cases:" << callset_ids_cases.size();
		}
		foreach (int ps_id, control_samples_)
		{
			//get sample type
			QString processing_system_type = db_.getProcessedSampleData(QString::number(ps_id)).processing_system_type;
			double min_correlation = (processing_system_type == "WGS")? 0.55: 0.9;
			cnv_callset_query.bindValue(0, ps_id);
			cnv_callset_query.exec();
			while(cnv_callset_query.next())
			{
				int callset_id = cnv_callset_query.value(0).toInt();
				QJsonDocument quality_metrics = QJsonDocument::fromJson(cnv_callset_query.value(1).toByteArray());
				double ref_correlation = quality_metrics.object().value("mean correlation to reference samples").toDouble();

				if (ref_correlation >= min_correlation) callset_ids_controls << callset_id;
			}
			qDebug() << "callset ids controls:" << callset_ids_controls.size();
		}

		//read polymorphism region
		QStringList igv_tracks = Settings::stringList("igv_menu"); //TODO Leon: seperate entry!
		foreach (const QString& track, igv_tracks)
		{
			QStringList columns = track.split('\t');
			if (columns.at(0).startsWith("Copy-number polymorphism regions"))
			{
				cnv_polymorphism_region.load(columns.at(2).trimmed());
			}
		}

	}
	ChromosomalIndex<BedFile> cnv_polymorphism_region_index(cnv_polymorphism_region);

	int i=0;
	double n_sec_single_query = 0;
	// perform search
	foreach (int gene_id, gene_ids)
	{
		i++;
		QByteArray gene_name = db_.geneSymbol(gene_id);

		// get gene region
		BedFile gene_regions;
		if(ui_->cb_only_ccr->isChecked())
		{
			//limit region only to CCR80
			if(ccr80_region_.contains(gene_name)) gene_regions = ccr80_region_.value(gene_name);
		}
		else
		{
			//use exon region
			gene_regions = db_.geneToRegions(gene_name, Transcript::ENSEMBL, "exon", true);

			//extend exon region by splice region
			gene_regions.extend(ui_->sb_splice_region_size->value());
		}

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

				detected_variants[ps_id] << var_id;
			}

		}



		QStringList ps_names_cases;
		QStringList ps_names_controls;

		int n_cases = countOccurences(variant_ids, case_samples_, detected_variants, inheritance, ps_names_cases);
		int n_controls = countOccurences(variant_ids, control_samples_, detected_variants, inheritance, ps_names_controls);

		//sort processed samples
		std::sort(ps_names_cases.begin(), ps_names_cases.end());
		std::sort(ps_names_controls.begin(), ps_names_controls.end());

		//init optional values
		double p_value;
		int n_cases_cnv = 0;
		int n_controls_cnv = 0;
		QStringList ps_names_cases_cnv;
		QStringList ps_names_controls_cnv;
		if (include_cnvs)
		{
			//get cnv counts
			if (callset_ids_cases.size() > 0 ) n_cases_cnv = countOccurencesCNV(callset_ids_cases, gene_regions, cnv_polymorphism_region, cnv_polymorphism_region_index, ps_names_cases_cnv);
			if (callset_ids_controls.size() > 0 ) n_controls_cnv = countOccurencesCNV(callset_ids_controls, gene_regions, cnv_polymorphism_region, cnv_polymorphism_region_index, ps_names_controls_cnv);
			//sort processed samples
			std::sort(ps_names_cases_cnv.begin(), ps_names_cases_cnv.end());
			std::sort(ps_names_controls_cnv.begin(), ps_names_controls_cnv.end());

			//get combined counts
			int n_cases_combined = (ps_names_cases.toSet() + ps_names_cases_cnv.toSet()).size();
			int n_controls_combined = (ps_names_controls.toSet() + ps_names_controls_cnv.toSet()).size();

			//calculate p-value (fisher) (SNPs only)
			p_value = BasicStatistics::fishersExactTest(n_cases_combined, n_controls_combined, case_samples_.size(), control_samples_.size(), "greater");
		}
		else
		{
			//calculate p-value (fisher) (SNPs only)
			p_value = BasicStatistics::fishersExactTest(n_cases, n_controls, case_samples_.size(), control_samples_.size(), "greater");
		}


		qDebug() << gene_name << "calculating counts took: " << Helper::elapsedTime(timer);




		//create table line
		int row_idx = ui_->tw_gene_table->rowCount();
		int column_idx = 0;
		ui_->tw_gene_table->setRowCount(row_idx+1);

		ui_->tw_gene_table->setItem(row_idx, column_idx++, GUIHelper::createTableItem(gene_name));
		ui_->tw_gene_table->setItem(row_idx, column_idx++, GUIHelper::createTableItem(p_value, -1));

		ui_->tw_gene_table->setItem(row_idx, column_idx, GUIHelper::createTableItem(n_cases));
		ui_->tw_gene_table->item(row_idx, column_idx++)->setToolTip(ps_names_cases.join(", "));
		ui_->tw_gene_table->setItem(row_idx, column_idx, GUIHelper::createTableItem(n_controls));
		ui_->tw_gene_table->item(row_idx, column_idx++)->setToolTip(ps_names_controls.join(", "));

		// add infos for hidden columns (ps names)
		ui_->tw_gene_table->setItem(row_idx, column_idx++, GUIHelper::createTableItem(ps_names_cases.join(", ")));
		ui_->tw_gene_table->setItem(row_idx, column_idx++, GUIHelper::createTableItem(ps_names_controls.join(", ")));

		if (include_cnvs)
		{
			ui_->tw_gene_table->setItem(row_idx, column_idx, GUIHelper::createTableItem(n_cases_cnv));
			ui_->tw_gene_table->item(row_idx, column_idx++)->setToolTip(ps_names_cases_cnv.join(", "));
			ui_->tw_gene_table->setItem(row_idx, column_idx, GUIHelper::createTableItem(n_controls_cnv));
			ui_->tw_gene_table->item(row_idx, column_idx++)->setToolTip(ps_names_controls_cnv.join(", "));

			// add infos for hidden columns (ps names)
			ui_->tw_gene_table->setItem(row_idx, column_idx++, GUIHelper::createTableItem(ps_names_cases_cnv.join(", ")));
			ui_->tw_gene_table->setItem(row_idx, column_idx++, GUIHelper::createTableItem(ps_names_controls_cnv.join(", ")));
		}

	}

	//final adjustments
	ui_->tw_gene_table->verticalHeader()->hide();
	ui_->tw_gene_table->setColumnHidden(4, true);
	ui_->tw_gene_table->setColumnHidden(5, true);
	if (include_cnvs)
	{
		ui_->tw_gene_table->setColumnHidden(8, true);
		ui_->tw_gene_table->setColumnHidden(9, true);
	}
	GUIHelper::resizeTableCellWidths(ui_->tw_gene_table, 200);
	GUIHelper::resizeTableCellHeightsToFirst(ui_->tw_gene_table);
	ui_->tw_gene_table->setSortingEnabled(true);
	ui_->tw_gene_table->sortByColumn(1, Qt::AscendingOrder);


	qDebug() << "Burden test took: " << Helper::elapsedTime(timer);
	QApplication::restoreOverrideCursor();
	test_running_ = false;
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
	comments << "splice_region_size=" + QString::number(ui_->sb_splice_region_size->value());

	//excluded regions
	comments << "excluded_regions=" + QString::number(excluded_regions_.count());
	comments << "excluded_bases=" + QString::number(excluded_regions_.baseCount());
	if(excluded_regions_file_names.size() > 0) comments << "excluded_region_file_names=" + excluded_regions_file_names.join(",");

	//CCR80
	if(ui_->cb_only_ccr->isChecked()) comments << "region_limited_to_CCR80=true";

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

	if(ui_->cb_include_cnvs->isChecked())
	{
		comments << "CNV_scaled_log_likelihood=15";
		comments << "CNV_reference_correlation>=0.9(WES)|>=0.55(WGS)";
		comments << "CNV_polymorphism_overlap<=0.95";
		comments << "CNV_copy_number=0";

	}

	GUIHelper::copyToClipboard(ui_->tw_gene_table, false, comments);
}

void BurdenTestWidget::copyWarningsToClipboard()
{
	GUIHelper::copyToClipboard(tw_warnings_, false);
}

void BurdenTestWidget::validateCCRGenes()
{
	//check only if CCR is checked
	if(!ui_->cb_only_ccr->isChecked()) return;

	GeneSet unsupported_genes;
	foreach(const QByteArray& gene, selected_genes_)
	{
		if(!ccr_genes_.contains(gene))
		{
			unsupported_genes.insert(gene);
		}
	}

	if(unsupported_genes.count() > 0)
	{
		GUIHelper::showMessage("Unsupported Genes", "The foillowing genes are not supported by the constrained coding region and will be removed:\n" + unsupported_genes.toStringList().join(", "));
		selected_genes_.remove(unsupported_genes);
		updateGeneCounts();
	}
}

void BurdenTestWidget::initCCR()
{

	//read CCR gene list and update the gene names
	ccr_genes_ = db_.genesToApproved(GeneSet::createFromFile(":/Resources/CCR_supported_genes.txt"));

	//create CCR region for each gene
	BedFile combined_ccr;
	combined_ccr.load(":/Resources/CCR80_GRCh38.bed");
	ccr80_region_.clear();
	for (int i=0; i<combined_ccr.count(); i++)
	{
		const BedLine& line = combined_ccr[i];
		QByteArray gene = db_.geneToApproved(line.annotations().at(1));
		//skip invalid genes
		if(gene.isEmpty()) continue;
		ccr80_region_[gene].append(BedLine(line.chr(), line.start(), line.end()));
	}
}

void BurdenTestWidget::updateCNVCheckbox()
{
	if(ui_->cb_inheritance->currentText().startsWith("recessive"))
	{
		ui_->cb_include_cnvs->setEnabled(true);
		ui_->cb_include_cnvs->setCheckState(Qt::Unchecked);
	}
	else
	{
		ui_->cb_include_cnvs->setEnabled(false);
		ui_->cb_include_cnvs->setCheckState(Qt::Unchecked);
	}
}


int BurdenTestWidget::countOccurences(const QSet<int>& variant_ids, const QSet<int>& ps_ids, const QMap<int, QSet<int>>& detected_variants, Inheritance inheritance, QStringList& ps_names)
{
	int n_hits = 0;
	foreach(int ps_id, ps_ids)
	{
		//check for variant in gene
		if(!detected_variants.contains(ps_id)) continue;
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
			int variant_id = intersection.toList().at(0);
			// check for hom var
			QString genotype = db_.getValue("SELECT genotype FROM detected_variant WHERE processed_sample_id=" + QString::number(ps_id) + " AND variant_id="
											+ QString::number(variant_id)).toString();

			// skip het vars (except het on chr X in male samples)
			if (genotype == "het")
			{
				QString gender = db_.getSampleData(db_.sampleId(db_.processedSampleName(QString::number(ps_id)))).gender;
				if (gender != "male") continue;
				Variant var = db_.variant(QString::number(variant_id));
				if (!var.chr().isX()) continue;
				BedFile par = NGSHelper::pseudoAutosomalRegion(GSvarHelper::build());
				if (!par.overlapsWith(var.chr(), var.start(), var.end())) continue;

				//else: variant is kept since it is a het variant on the chr X of a male sample
			}
		}
		//else: at least two hits or non-ressesive:
		n_hits++;
		ps_names << db_.processedSampleName(QString::number(ps_id));
	}

	return n_hits;
}

int BurdenTestWidget::
countOccurencesCNV(const QSet<int>& callset_ids, const BedFile& regions, const BedFile& cnv_polymorphism_region, const ChromosomalIndex<BedFile>& cnv_polymorphism_region_index, QStringList& ps_names)
{
	ps_names.clear();

	//debug logs:
	int skipped_wrong_cn = 0;
	int skipped_low_logll = 0;
	int skipped_overlap_pmr = 0;
	//get all cnvs intersecting the given region
	//create query
	QString query_text = QString("SELECT id FROM cnv WHERE ");

	//filter by callset
	QStringList callset_str;
	foreach (int c_id, callset_ids)
	{
		callset_str << QString::number(c_id);
	}
	query_text += "cnv_callset_id IN (" +  callset_str.join(", ") + ") AND ";

	//filter by region
	QStringList chr_ranges;
	for (int i = 0; i < regions.count(); ++i)
	{
		chr_ranges << "(chr='" + regions[i].chr().strNormalized(true) + "' AND end>=" + QString::number(regions[i].start()) + " AND start<=" + QString::number(regions[i].end()) + ")";
	}
	query_text += "(" + chr_ranges.join(" OR ") + ")";

	//execute
	QList<int> cnv_ids = db_.getValuesInt(query_text);

	//filter down variants
	foreach (int cnv_id, cnv_ids)
	{
		//filter by CN
		int cn = db_.getValue("SELECT cn FROM cnv WHERE id=:0", false, QString::number(cnv_id)).toInt();
		if (cn != 0)
		{
			skipped_wrong_cn++;
			continue;
		}

		//filter by logll
		QJsonDocument json = QJsonDocument::fromJson(db_.getValue("SELECT cn FROM cnv WHERE id=:0", false, QString::number(cnv_id)).toByteArray());
		int ll = json.object().value("loglikelihood").toInt();
		int n_regions = (json.object().contains("regions"))?json.object().value("regions").toInt():json.object().value("no_of_regions").toInt();
		double scaled_ll = (double) ll / n_regions;
		if (scaled_ll < 15.0)
		{
			skipped_low_logll++;
			continue;
		}

		//filter by polymorphim region
		CopyNumberVariant cnv = db_.cnv(cnv_id);
		QVector<int> indices = cnv_polymorphism_region_index.matchingIndices(cnv.chr(), cnv.start(), cnv.end());
		BedFile overlap_regions;
		foreach (int idx, indices)
		{
			const BedLine& match = cnv_polymorphism_region[idx];
			overlap_regions.append(BedLine(cnv.chr(), std::max(cnv.start(),match.start()), std::min(cnv.end(),match.end())));
		}
		overlap_regions.sort();
		overlap_regions.merge();
		double overlap = (double) overlap_regions.baseCount() / cnv.size();
		if(overlap > 0.95)
		{
			skipped_overlap_pmr++;
			continue;
		}

		int ps_id = db_.getValue("SELECT cc.processed_sample_id FROM cnv c INNER JOIN cnv_callset cc ON cc.id=c.cnv_callset_id WHERE c.id=:0", false, QString::number(cnv_id)).toInt();
		ps_names.append(db_.processedSampleName(QString::number(ps_id)));
	}


	//report results
	ps_names.removeDuplicates();
	return ps_names.size();
}
