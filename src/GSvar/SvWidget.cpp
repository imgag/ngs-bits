#include <QFileInfo>
#include <QLabel>
#include <QMessageBox>
#include <QClipboard>
#include <QBitArray>
#include <QByteArray>
#include <QMenu>
#include <QAction>
#include <QCompleter>
#include <QDesktopServices>
#include "SvWidget.h"
#include "ui_SvWidget.h"
#include "Helper.h"
#include "GUIHelper.h"
#include "PhenotypeSelectionWidget.h"
#include "Settings.h"
#include "Log.h"
#include "LoginManager.h"
#include "GeneInfoDBs.h"
#include "VariantTable.h"
#include "ReportVariantDialog.h"
#include "SvSearchWidget.h"
#include "VariantDetailsDockWidget.h"
#include "ValidationDialog.h"
#include "SomaticReportVariantDialog.h"
#include "GlobalServiceProvider.h"
#include "IgvSessionManager.h"
#include "ClinvarUploadDialog.h"
#include "GSvarHelper.h"
#include "SettingsDialog.h"

SvWidget::SvWidget(QWidget* parent, const BedpeFile& bedpe_file, QString ps_id, QSharedPointer<ReportConfiguration> rep_conf, const GeneSet& het_hit_genes)
	: QWidget(parent)
	, ui(new Ui::SvWidget)
	, svs_(bedpe_file)
	, ps_id_(ps_id)
	, var_het_genes_(het_hit_genes)
	, report_config_(rep_conf)
    , ngsd_user_logged_in_(LoginManager::active())
    , rc_enabled_(ngsd_user_logged_in_ && report_config_!=nullptr && !report_config_->isFinalized())
	, is_somatic_(false)
	, is_multisample_(bedpe_file.format()==BedpeFileFormat::BEDPE_GERMLINE_TRIO || bedpe_file.format()==BedpeFileFormat::BEDPE_GERMLINE_MULTI)
{
	setupUI();

	initGUI();
}

SvWidget::SvWidget(QWidget* parent, const BedpeFile& bedpe_file, SomaticReportConfiguration& som_rep_conf, const GeneSet& het_hit_genes)
	: QWidget(parent)
	, ui(new Ui::SvWidget)
	, svs_(bedpe_file)
	, ps_id_()
	, var_het_genes_(het_hit_genes)
	, som_report_config_(&som_rep_conf)
    , ngsd_user_logged_in_(LoginManager::active())
    , rc_enabled_(ngsd_user_logged_in_ && som_report_config_!=nullptr)
	, is_somatic_(true)
	, is_multisample_(false)
{
	setupUI();

	//Disable filters that cannot apply for tumor normal pairs (data is expanded already)
	if(svs_.format() == BedpeFileFormat::BEDPE_SOMATIC_TUMOR_NORMAL)
	{
		ui->info_a->setEnabled(false);
		ui->info_b->setEnabled(false);
		ui->sv_details->setEnabled(false);
	}

	initGUI();
}

SvWidget::~SvWidget()
{
	delete ui;
}


void SvWidget::setupUI()
{
	ui->setupUi(this);

	GUIHelper::styleSplitter(ui->splitter);
	ui->splitter->setStretchFactor(0, 10);
	ui->splitter->setStretchFactor(1, 1);

	//Setup signals and slots
	connect(ui->copy_to_clipboard,SIGNAL(clicked()),this,SLOT(copyToClipboard()));
	connect(ui->svs,SIGNAL(itemDoubleClicked(QTableWidgetItem*)),this,SLOT(svDoubleClicked(QTableWidgetItem*)));
	connect(ui->svs,SIGNAL(itemSelectionChanged()),this,SLOT(updateFormatAndInfoTables()));
	connect(ui->svs,SIGNAL(customContextMenuRequested(QPoint)),this,SLOT(showContextMenu(QPoint)));
	connect(ui->filter_widget, SIGNAL(filtersChanged()), this, SLOT(applyFilters()));
	connect(ui->filter_widget, SIGNAL(phenotypeImportNGSDRequested()), this, SLOT(importPhenotypesFromNGSD()));
	connect(ui->filter_widget, SIGNAL(targetRegionChanged()), this, SLOT(clearTooltips()));
	connect(ui->filter_widget, SIGNAL(calculateGeneTargetRegionOverlap()), this, SLOT(annotateTargetRegionGeneOverlap()));
	connect(ui->svs->verticalHeader(), SIGNAL(sectionDoubleClicked(int)), this, SLOT(svHeaderDoubleClicked(int)));
	ui->svs->verticalHeader()->setContextMenuPolicy(Qt::CustomContextMenu);
	connect(ui->svs->verticalHeader(), SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(svHeaderContextMenu(QPoint)));

	ui->resize_btn->setMenu(new QMenu());
	ui->resize_btn->menu()->addAction("Open column settings", this, SLOT(openColumnSettings()));
	ui->resize_btn->menu()->addAction("Apply column width settings", this, SLOT(adaptColumnWidthsAndHeights()));
	ui->resize_btn->menu()->addAction("Show all columns", this, SLOT(showAllColumns()));
}

void SvWidget::initGUI()
{
	//clear GUI
	clearGUI();

	if(svs_.count() == 0)
	{
		disableGUI("There are no SVs in the data file");
		return;
	}

	//determine processed sample names
	const SampleHeaderInfo& header_info = svs_.sampleHeaderInfo();
	foreach (const SampleInfo& sample_info, header_info)
	{
		ps_names_ << sample_info.name;
	}

	//add genotype of samples as separate columns (FORMAT/SAMPLE columns are not shown by default)
	int col_idx = ui->svs->columnCount();
	ui->svs->setColumnCount(ui->svs->columnCount() + ps_names_.size());
	for (int i=0; i<ps_names_.size(); ++i)
	{
		QTableWidgetItem* item = new QTableWidgetItem(ps_names_.at(i));

		bool is_affected = i<header_info.count() ? header_info[i].isAffected() : false;
		if (svs_.format()==BedpeFileFormat::BEDPE_GERMLINE_TRIO)
		{
			item->setToolTip((QStringList() << "child" << "father" << "mother").at(i));
		}
		else
		{
			item->setToolTip(is_affected ? "affected" : "control");
		}
		if (is_multisample_ && is_affected) item->setForeground(QBrush(Qt::darkRed));

		ui->svs->setHorizontalHeaderItem(col_idx + i, item);
	}

	//determine column order
	QStringList col_order;
	QList<int> anno_index_order;
	ColumnConfig config = ColumnConfig::fromString(Settings::string("column_config_sv", true));
	config.getOrder(svs_, col_order, anno_index_order);

	//Add annotation headers
	foreach(QString col, col_order)
	{
		ui->svs->setColumnCount(ui->svs->columnCount() + 1);
		QTableWidgetItem* item = new QTableWidgetItem(col);

		if (col=="OMIM")
		{
			item->setIcon(QIcon("://Icons/Table.png"));
			item->setToolTip("Double click table cell to open table view of annotations");
		}

		QByteArray desc = svs_.annotationDescriptionByName(col.toUtf8()).trimmed();
		if(desc!="")
		{
			item->setToolTip(desc);
		}

		ui->svs->setHorizontalHeaderItem(ui->svs->columnCount() - 1, item);
	}

	//fill rows
	ui->svs->setRowCount(svs_.count());

	//get report variant indices
	QSet<int> report_variant_indices;
    if((report_config_ != NULL) && !is_somatic_) report_variant_indices = LIST_TO_SET(report_config_->variantIndices(VariantType::SVS, false));
    if((som_report_config_ != NULL) && is_somatic_) report_variant_indices = LIST_TO_SET(som_report_config_->variantIndices(VariantType::SVS, false));

	//fill table widget with data from bedpe file
	for(int row=0; row<svs_.count(); ++row)
	{
		//Set vertical header
		QTableWidgetItem* header_item = GUIHelper::createTableItem(QByteArray::number(row+1));
		if (report_variant_indices.contains(row))
		{
			if (! is_somatic_)
			{
				const ReportVariantConfiguration& rc = report_config_->get(VariantType::SVS, row);
				header_item->setIcon(VariantTable::reportIcon(rc.showInReport(), rc.causal));
			}
			else if (som_report_config_ != NULL)
			{
				const SomaticReportVariantConfiguration& rc = som_report_config_->get(VariantType::SVS, row);
				header_item->setIcon(VariantTable::reportIcon(rc.showInReport(), false));
			}
		}
		ui->svs->setVerticalHeaderItem(row, header_item);

		//set fixed columns
		ui->svs->setItem(row, 0, new QTableWidgetItem(QString(svs_[row].chr1().str())));
		ui->svs->setItem(row, 1, new QTableWidgetItem(QString::number(svs_[row].start1())));
		ui->svs->setItem(row, 2, new QTableWidgetItem(QString::number(svs_[row].end1())));

		ui->svs->setItem(row, 3, new QTableWidgetItem(QString(svs_[row].chr2().str())));
		ui->svs->setItem(row, 4, new QTableWidgetItem(QString::number(svs_[row].start2())));
		ui->svs->setItem(row, 5, new QTableWidgetItem(QString::number(svs_[row].end2())));

		int col_in_widget = 6;

		//set genotypes of samples
		for (int idx_sample = 0; idx_sample < ps_names_.size(); ++idx_sample)
		{
			QString gt = svs_[row].genotypeHumanReadable(svs_.annotationHeaders(), false, idx_sample);
			ui->svs->setItem(row, col_in_widget++, new QTableWidgetItem(gt));
		}

		//set annotation columns
		foreach(int anno_index, anno_index_order)
		{
			ui->svs->setItem(row, col_in_widget, new QTableWidgetItem(QString(svs_[row].annotations().at(anno_index))));
			++col_in_widget;
		}
	}

	//add sample names to details view for multi-sample
	if (is_multisample_)
	{
		ui->sv_details->setColumnCount(2 + ps_names_.size());
		for (int sample_idx = 0; sample_idx < ps_names_.size(); ++sample_idx)
		{
			ui->sv_details->setHorizontalHeaderItem(2 + sample_idx, new QTableWidgetItem(QString(ps_names_.at(sample_idx))));
		}
	}

	//set entries for SV filter columns filter
	QStringList valid_filter_entries;
	foreach (const QByteArray& entry, svs_.metaInfoDescriptionByID("FILTER").keys())
	{
		valid_filter_entries.append(entry);
	}
	valid_filter_entries.removeAll("PASS");
	valid_filter_entries.prepend("PASS");
	ui->filter_widget->setValidFilterEntries(valid_filter_entries);

	//adapt cells to contents
	adaptColumnWidthsAndHeights();

	//hide columns if requested
	config.applyHidden(ui->svs);

	//filter rows according default filter values
	applyFilters();
}


void SvWidget::clearGUI()
{
	ui->label_size->clear();
	ui->label_pe_af->clear();
	ui->label_sr_af->clear();
	ui->number_of_svs->clear();

	//clear info widgets
	ui->info_a->setRowCount(0);
	ui->info_b->setRowCount(0);
	ui->sv_details->setRowCount(0);

	//clear table widget to cols / rows specified in .ui file
	ui->svs->setRowCount(0);
	ui->svs->setColumnCount(6);

}

void SvWidget::resizeTableContent(QTableWidget* table_widget)
{
	GUIHelper::resizeTableCellWidths(table_widget, 200);
	GUIHelper::resizeTableCellHeightsToFirst(table_widget);
}


void SvWidget::applyFilters(bool debug_time)
{
	//skip if not necessary
	int row_count = ui->svs->rowCount();
	if (row_count==0) return;

	// filters based on FilterWidgetSV
	FilterResult filter_result(row_count);
	try
	{
		QApplication::setOverrideCursor(Qt::BusyCursor);

        QElapsedTimer timer;
		timer.start();

		// filter by FilterCascade
		const FilterCascade& filter_cascade = ui->filter_widget->filters();

		//set comp-het gene list the first time the filter is applied
		for(int i=0; i<filter_cascade.count(); ++i)
		{
			const FilterSvCompHet* comphet_filter = dynamic_cast<const FilterSvCompHet*>(filter_cascade[i].data());
			if (comphet_filter!=nullptr && comphet_filter->hetHitGenes().count()!=var_het_genes_.count())
			{
				comphet_filter->setHetHitGenes(var_het_genes_);
			}
		}

		// apply filters of the filter cascade
		filter_result = filter_cascade.apply(svs_, false, debug_time);
		ui->filter_widget->markFailedFilters();

		if (debug_time)
		{
			Log::perf("Applying annotation filters took ", timer);
			timer.start();
		}

		//filter by report config
		ReportConfigFilter rc_filter = ui->filter_widget->reportConfigurationFilter();
		if (rc_filter!=ReportConfigFilter::NONE)
		{
			for(int row=0; row<row_count; ++row)
			{
				if (!filter_result.flags()[row]) continue;

				if (rc_filter==ReportConfigFilter::HAS_RC)
				{
					if(is_somatic_)
					{
						filter_result.flags()[row] = som_report_config_->exists(VariantType::SVS, row);
					}
					else
					{
						filter_result.flags()[row] = report_config_->exists(VariantType::SVS, row);
					}
				}
				else if (rc_filter==ReportConfigFilter::NO_RC)
				{
					if(is_somatic_)
					{
						filter_result.flags()[row] = !som_report_config_->exists(VariantType::SVS, row);
					}
					else
					{
						filter_result.flags()[row] = !report_config_->exists(VariantType::SVS, row);
					}
				}
			}
		}

		//filter by ROI
		if (ui->filter_widget->targetRegion().isValid())
		{
			for(int row=0; row<row_count; ++row)
			{
				if(!filter_result.flags()[row]) continue;

				if (!svs_[row].intersectsWith(ui->filter_widget->targetRegion().regions, true)) filter_result.flags()[row] = false;
			}
		}

		//filter by region
		QString region_text = ui->filter_widget->region();
		BedLine region = BedLine::fromString(region_text);
		if (!region.isValid()) //check if valid chr
		{
			Chromosome chr(region_text);
			if (chr.isNonSpecial())
			{
				region.setChr(chr);
				region.setStart(1);
				region.setEnd(999999999);
			}
		}
		if (region.isValid()) //valid region (chr,start, end or only chr)
		{
			for(int row=0; row<row_count; ++row)
			{
				if (!filter_result.flags()[row]) continue;

				const BedpeLine& sv = svs_[row];
				filter_result.flags()[row] = sv.intersectsWith(BedFile(region.chr(), region.start(), region.end()), true);
			}
		}


		//filter by phenotype (via genes, not genomic regions)
		PhenotypeList phenotypes = ui->filter_widget->phenotypes();
		if (!phenotypes.isEmpty())
		{
			NGSD db;
			//convert phenotypes to genes
			GeneSet pheno_genes;
            for (const Phenotype& pheno : phenotypes)
			{
				pheno_genes << db.phenotypeToGenes(db.phenotypeIdByAccession(pheno.accession()), true);
			}

			//convert genes to ROI (using a cache to speed up repeating queries)
			BedFile pheno_roi;
            for (const QByteArray& gene : pheno_genes)
			{
				pheno_roi.add(GlobalServiceProvider::geneToRegions(gene, db));
			}
			pheno_roi.merge();

			for(int row=0; row<row_count; ++row)
			{
				if (!filter_result.flags()[row]) continue;

				filter_result.flags()[row] = svs_[row].intersectsWith(pheno_roi, true);
			}
		}

		//filter by genes
		GeneSet gene_whitelist = ui->filter_widget->genes();
		if (!gene_whitelist.isEmpty())
		{
			QByteArray genes_joined = gene_whitelist.join('|');

			// get column index of 'GENES' column
			int i_genes = svs_.annotationIndexByName("GENES", false);
			if (i_genes == -1)
			{
				QMessageBox::warning(this, "Filtering error", "BEDPE files does not contain a 'GENES' column! \nFiltering based on genes is not possible. Please reannotate the structural variant file.");
			}
			else
			{
				if (genes_joined.contains("*")) //with wildcards
				{
                    QRegularExpression reg(genes_joined.replace("-", "\\-").replace("*", "[A-Z0-9-]*"));
					for(int row=0; row<row_count; ++row)
					{
						if (!filter_result.flags()[row]) continue;

						// generate GeneSet from column text
						GeneSet sv_genes = GeneSet::createFromText(svs_[row].annotations()[i_genes], ',');

						bool match_found = false;
                        for (const QByteArray& sv_gene : sv_genes)
						{
                            if (reg.match(sv_gene).hasMatch())
							{
								match_found = true;
								break;
							}
						}
						filter_result.flags()[row] = match_found;
					}
				}
				else //without wildcards
				{
					for(int row=0; row<row_count; ++row)
					{
						if (!filter_result.flags()[row]) continue;

						// generate GeneSet from column text
						GeneSet sv_genes = GeneSet::createFromText(svs_[row].annotations()[i_genes], ',');

						filter_result.flags()[row] = sv_genes.intersectsWith(gene_whitelist);
					}
				}
			}
		}

		//filter annotations by text
		QByteArray text = ui->filter_widget->text().trimmed().toLower();
		if (text!="")
		{
			for(int row=0; row<row_count; ++row)
			{
				if (!filter_result.flags()[row]) continue;

				bool match = false;
				foreach(const QByteArray& anno, svs_[row].annotations())
				{
					if (anno.toLower().contains(text))
					{
						match = true;
						break;
					}
				}
				filter_result.flags()[row] = match;
			}
		}

		QApplication::restoreOverrideCursor();
	}
	catch(Exception& e)
	{
		GUIHelper::showException(this, e, "Filtering error");

		filter_result = FilterResult(row_count, false);
	}

	//hide rows not passing filters
	for(int row=0; row<row_count; ++row)
	{
		ui->svs->setRowHidden(row, !filter_result.flags()[row]);
	}

	//Set number of filtered / total SVs
	ui->number_of_svs->setText(QByteArray::number(filter_result.flags().count(true)) + "/" + QByteArray::number(row_count));
}

double SvWidget::alleleFrequency(int row, QByteArray sample, QByteArray read_type)
{
	if (read_type!="SR" && read_type!="PR") THROW(ArgumentException, "Invalid read type '"+read_type+"' given in SvWidget::alleleFrequency!");

	try
	{
		//get sample data
		int i_format = svs_.annotationIndexByName("FORMAT");
		int i_sample = svs_.annotationIndexByName(sample);
		QByteArrayList values = svs_[row].getSampleFormatData(i_format, i_sample, read_type).split(',');
		if(values.count()!=2) THROW(ArgumentException, "Value of '"+read_type+"' could not be split in two parts!");

		//get counts
		int count_ref = Helper::toInt(values[0], "ref count");
		int count_alt = Helper::toInt(values[1], "alt count");
		if(count_alt+count_ref==0) return 0;

		return (double)count_alt / (count_alt+count_ref);
	}
	catch (Exception& e)
	{
		return -1;
	}
}

void SvWidget::editSvValidation(int row)
{
	BedpeLine& sv = svs_[row];

	try
	{
		NGSD db;

		//get SV ID
		QString callset_id = db.getValue("SELECT id FROM sv_callset WHERE processed_sample_id=:0", true, ps_id_).toString();
		if (callset_id == "") THROW(DatabaseException, "No callset found for processed sample id " + ps_id_ + "!");
		QString sv_id = db.svId(sv, Helper::toInt(callset_id, "callset_id"), svs_);
		if (sv_id == "") THROW(DatabaseException, "SV not found in the NGSD! ");


		//get sample ID
		QString sample_id = db.sampleId(db.processedSampleName(ps_id_));

		//get variant validation ID - add if missing
		QVariant val_id = db.getValue("SELECT id FROM variant_validation WHERE "+ db.svTableName(sv.type()) + "_id='" + sv_id + "' AND sample_id='" + sample_id + "'", true);
		bool added_validation_entry = false;
		if (!val_id.isValid())
		{
			//insert
			SqlQuery query = db.getQuery();
			query.exec("INSERT INTO variant_validation (user_id, sample_id, variant_type, " + db.svTableName(sv.type()) + "_id, status) VALUES ('" + LoginManager::userIdAsString() + "','" + sample_id + "','SV','" + sv_id + "','n/a')");
			val_id = query.lastInsertId();

			added_validation_entry = true;
		}

		ValidationDialog dlg(this, val_id.toInt());

		if (dlg.exec())
		{
			//update DB
			dlg.store();
		}
		else if (added_validation_entry)
		{
			// remove created but empty validation if ValidationDialog is aborted
			SqlQuery query = db.getQuery();
			query.exec("DELETE FROM variant_validation WHERE id=" + val_id.toString());
		}
	}
	catch (DatabaseException& e)
	{
		GUIHelper::showMessage("NGSD error", e.message());
		return;
	}
}

void SvWidget::editGermlineReportConfiguration(int row)
{
	if(report_config_ == nullptr)
	{
		THROW(ProgrammingException, "ReportConfiguration in SvWidget is nullpointer.");
	}

	if(!svs_[row].chr1().isNonSpecial() || !svs_[row].chr2().isNonSpecial())
	{
		QMessageBox::warning(this, "Error adding SV", "Structural variants from special chromosomes cannot be imported into the NGSD!");
		return;
	}

	NGSD db;

	//init/get config
	ReportVariantConfiguration var_config;
	if (report_config_->exists(VariantType::SVS, row))
	{
		var_config = report_config_->get(VariantType::SVS, row);
	}
	else
	{
		var_config.variant_type = VariantType::SVS;
		var_config.variant_index = row;
	}

	//get inheritance mode by gene
	QList<KeyValuePair> inheritance_by_gene;
	int i_genes = svs_.annotationIndexByName("genes", false);
	if (i_genes!=-1)
	{
		GeneSet genes = GeneSet::createFromText(svs_[row].annotations()[i_genes], ',');
		foreach(const QByteArray& gene, genes)
		{

			GeneInfo gene_info = db.geneInfo(gene);
			inheritance_by_gene << KeyValuePair{gene, gene_info.inheritance};
		}
	}

	//exec dialog
	ReportVariantDialog dlg(svs_[row].toString(), inheritance_by_gene, var_config, this);
	dlg.setEnabled(!report_config_->isFinalized());
	if (dlg.exec()!=QDialog::Accepted) return;

	//update config, GUI and NGSD
	report_config_->set(var_config);
	updateReportConfigHeaderIcon(row);
}

void SvWidget::editSomaticReportConfiguration(int row)
{
	if(som_report_config_ == nullptr)
	{
		THROW(ProgrammingException, "SomaticReportConfiguration in SvWidget is nullpointer.");
	}

	if(!svs_[row].chr1().isNonSpecial() || !svs_[row].chr2().isNonSpecial())
	{
		QMessageBox::warning(this, "Error adding SV", "Structural variants from special chromosomes cannot be imported into the NGSD!");
		return;
	}

	NGSD db;

	//init/get config
	SomaticReportVariantConfiguration var_config;
	if (som_report_config_->exists(VariantType::SVS, row))
	{
		var_config = som_report_config_->get(VariantType::SVS, row);
	}
	else
	{
		var_config.variant_type = VariantType::SVS;
		var_config.variant_index = row;
	}

	//exec dialog
	SomaticReportVariantDialog dlg(svs_[row].toString(), var_config, this);
	dlg.setEnabled(true);
	if (dlg.exec()!=QDialog::Accepted) return;

	//update config, GUI and NGSD
	som_report_config_->addSomaticVariantConfiguration(var_config);
	emit updateSomaticReportConfiguration();
	updateReportConfigHeaderIcon(row);
}

void SvWidget::uploadToClinvar(int index1, int index2)
{
    if (!ngsd_user_logged_in_) return;

	try
	{
		//abort if 1st index is missing
		if(index1 <0)
		{
			THROW(ArgumentException, "A valid index for the first SV has to be provided!");
		}
		//abort if API key is missing
		if(Settings::string("clinvar_api_key", true).trimmed().isEmpty())
		{
			THROW(ProgrammingException, "ClinVar API key is needed, but not found in settings.\nPlease inform the bioinformatics team");
		}

		NGSD db;

		//(1) prepare data as far as we can
		ClinvarUploadData data;
		data.processed_sample = db.processedSampleName(ps_id_);
		data.variant_type1 = VariantType::SVS;
		if(index2 < 0)
		{
			//Single variant submission
			data.submission_type = ClinvarSubmissionType::SingleVariant;
			data.variant_type2 = VariantType::INVALID;
		}
		else
		{
			//CompHet variant submission
			data.submission_type = ClinvarSubmissionType::CompoundHeterozygous;
			data.variant_type2 = VariantType::SVS;
		}

		QString sample_id = db.sampleId(data.processed_sample);
		SampleData sample_data = db.getSampleData(sample_id);

		//get disease info
		data.disease_info = db.getSampleDiseaseInfo(sample_id, "OMIM disease/phenotype identifier");
		data.disease_info.append(db.getSampleDiseaseInfo(sample_id, "Orpha number"));
		if (data.disease_info.length() < 1)
		{
			INFO(InformationMissingException, "The sample has to have at least one OMIM or Orphanet disease identifier to publish a variant in ClinVar.");
		}

		// get affected status
		data.affected_status = sample_data.disease_status;

		//get phenotype(s)
		data.phenos = sample_data.phenotypes;

		//get sv variant info
		data.sv1 = svs_[index1];
		if(data.sv1.type() == StructuralVariantType::BND)
			WARNING(NotImplementedException, "The upload of translocations is not supported by the ClinVar API. Please use the manual submission through the ClinVar website.");

		if(data.submission_type == ClinvarSubmissionType::CompoundHeterozygous)
		{
			data.sv2 = svs_[index2];
			if(data.sv2.type() == StructuralVariantType::BND)
				WARNING(NotImplementedException, "The upload of translocations is not supported by the ClinVar API. Please use the manual submission through the ClinVar website.");
		}

		// get report info
		if (!report_config_.data()->exists(VariantType::SVS, index1))
		{
			INFO(InformationMissingException, "The SV has to be in the report configuration to be published!");
		}
		data.report_variant_config1 = report_config_.data()->get(VariantType::SVS, index1);
		if(data.submission_type == ClinvarSubmissionType::CompoundHeterozygous)
		{
			if (!report_config_.data()->exists(VariantType::SVS, index2))
			{
				INFO(InformationMissingException, "The SV 2 has to be in the report configuration to be published!");
			}
			data.report_variant_config2 = report_config_.data()->get(VariantType::SVS, index2);
		}

		//check classification
		if (data.report_variant_config1.classification.trimmed().isEmpty() || (data.report_variant_config1.classification.trimmed() == "n/a"))
		{
			INFO(InformationMissingException, "The SV has to be classified to be published!");
		}
		if(data.submission_type == ClinvarSubmissionType::CompoundHeterozygous)
		{
			if (data.report_variant_config2.classification.trimmed().isEmpty() || (data.report_variant_config2.classification.trimmed() == "n/a"))
			{
				INFO(InformationMissingException, "The SV 2 has to be classified to be published!");
			}
		}

		//genes
		data.genes = data.sv1.genes(svs_.annotationHeaders());
		if(data.submission_type == ClinvarSubmissionType::CompoundHeterozygous) data.genes <<  data.sv2.genes(svs_.annotationHeaders());

		//get callset id
		QString callset_id = db.getValue("SELECT id FROM sv_callset WHERE processed_sample_id=:0", true, ps_id_).toString();
		if (callset_id == "") THROW(DatabaseException, "No callset found for processed sample id " + ps_id_ + "!");

		//determine NGSD ids of variant and report variant for variant 1
		QString sv_id = db.svId(data.sv1, callset_id.toInt(), svs_, false);
		if (sv_id == "")
		{
			INFO(InformationMissingException, "The SV has to be in NGSD and part of a report config to be published!");
		}


		data.variant_id1 = Helper::toInt(sv_id);
		//extract report variant id
		int rc_id = db.reportConfigId(ps_id_);
		if (rc_id == -1 )
		{
			THROW(DatabaseException, "Could not determine report config id for sample " + data.processed_sample + "!");
		}

		data.report_variant_config_id1 = db.getValue("SELECT id FROM report_configuration_sv WHERE report_configuration_id=" + QString::number(rc_id) + " AND "
											 + db.svTableName(data.sv1.type())+ "_id=" + QString::number(data.variant_id1), false).toInt();

		if(data.submission_type == ClinvarSubmissionType::CompoundHeterozygous)
		{
			//determine NGSD ids of sv for variant 2
			sv_id = db.svId(data.sv2, callset_id.toInt(), svs_, true);
			if (sv_id == "")
			{
				INFO(InformationMissingException, "The SV 2 has to be in NGSD and part of a report config to be published!");
			}
			data.variant_id2 = Helper::toInt(sv_id);

			//extract report variant id
			data.report_variant_config_id2 = db.getValue("SELECT id FROM report_configuration_sv WHERE report_configuration_id=" + QString::number(rc_id) + " AND "
												 + db.svTableName(data.sv2.type())+ "_id=" + QString::number(data.variant_id2), false).toInt();
		}


		// (2) show dialog
		ClinvarUploadDialog dlg(this);
		dlg.setData(data);
		dlg.exec();



	}
	catch(Exception& e)
	{
		GUIHelper::showException(this, e, "ClinVar submission error");
	}
}

void SvWidget::annotateTargetRegionGeneOverlap()
{
	QApplication::setOverrideCursor(Qt::BusyCursor);

	//generate gene regions and index
	BedFile roi_genes = NGSD().genesToRegions(ui->filter_widget->targetRegion().genes, Transcript::ENSEMBL, "gene", true);
	roi_genes.extend(5000);
	ChromosomalIndex<BedFile> roi_genes_index(roi_genes);

	// update gene tooltips
	int gene_idx = GUIHelper::columnIndex(ui->svs, "GENES", false);
	if (gene_idx!=-1)
	{
		// iterate over sv table
		for (int row_idx = 0; row_idx < ui->svs->rowCount(); ++row_idx)
		{
			// get all matching lines in gene bed file
			BedFile sv_regions = svs_[row_idx].affectedRegion();
			QVector<int> matching_indices;
			for (int i = 0; i < sv_regions.count(); ++i)
			{
				matching_indices << roi_genes_index.matchingIndices(sv_regions[i].chr(), sv_regions[i].start(), sv_regions[i].end());
			}

			// extract gene names
			GeneSet genes;
			foreach (int idx, matching_indices)
			{
				genes.insert(roi_genes[idx].annotations().at(0));
			}

			// update tooltip
			ui->svs->item(row_idx, gene_idx)->setToolTip("<div style=\"wordwrap\">Target region gene overlap: <br> " + genes.toStringList().join(", ") + "</div>");
		}
	}

	QApplication::restoreOverrideCursor();
}

void SvWidget::clearTooltips()
{
	int gene_idx = GUIHelper::columnIndex(ui->svs, "GENES", false);
	if (gene_idx!=-1)
	{
		for (int row_idx = 0; row_idx < ui->svs->rowCount(); ++row_idx)
		{
			// remove tool tip
			ui->svs->item(row_idx, gene_idx)->setToolTip("");
		}
	}
}

void SvWidget::copyToClipboard()
{
	GUIHelper::copyToClipboard(ui->svs);
}


void SvWidget::svDoubleClicked(QTableWidgetItem *item)
{
	if (item==nullptr) return;

	int col = item->column();
	int row = item->row();

	QString col_name = item->tableWidget()->horizontalHeaderItem(col)->text();
	if (col_name=="OMIM")
	{
		int omim_index = svs_.annotationHeaders().indexOf("OMIM");

		QString text = svs_[row].annotations()[omim_index].trimmed();
		if (text.trimmed()!="")
		{
			VariantDetailsDockWidget::showOverviewTable("OMIM entries of ROH " +  svs_[row].toString(), text, ';', "https://www.omim.org/entry/");
		}
	}
	else
	{
		QString coords = svs_[row].positionRange();
        IgvSessionManager::get(0).gotoInIGV(coords, true);
	}
}

void SvWidget::disableGUI(const QString& message)
{
	setEnabled(false);

	ui->error_label->setText("<font color='red'>" + message + "</font>");
}

void SvWidget::importPhenotypesFromNGSD()
{
	if (ps_id_ == "")
	{
		QMessageBox::warning(this, "Error loading phenotypes", "Cannot load phenotypes because no processed sample ID is set!");
		return;
	}

	NGSD db;
	QString sample_id = db.getValue("SELECT sample_id FROM processed_sample WHERE id=:0", false, ps_id_).toString();
	PhenotypeList phenotypes = db.getSampleData(sample_id).phenotypes;

	ui->filter_widget->setPhenotypes(phenotypes);
}

void SvWidget::svHeaderDoubleClicked(int row)
{
	if (!rc_enabled_) return;
	editReportConfiguration(row);
}

void SvWidget::svHeaderContextMenu(QPoint pos)
{
	if (!rc_enabled_) return;

	//get variant index
	int row = ui->svs->verticalHeader()->visualIndexAt(pos.ry());

	//set up menu
	QMenu menu(ui->svs->verticalHeader());
	QAction* a_edit = menu.addAction(QIcon(":/Icons/Report.png"), "Add/edit report configuration");
	QAction* a_delete = menu.addAction(QIcon(":/Icons/Remove.png"), "Delete report configuration");
	if (is_somatic_)
	{
		a_delete->setEnabled(som_report_config_->exists(VariantType::SVS, row));
	}
	else
	{
		a_delete->setEnabled(!report_config_->isFinalized() && report_config_->exists(VariantType::SVS, row));
	}

	//exec menu
	pos = ui->svs->verticalHeader()->viewport()->mapToGlobal(pos);
	QAction* action = menu.exec(pos);
	if (action==nullptr) return;


	//actions
	if (action==a_edit)
	{
		editReportConfiguration(row);
	}
	else if (action==a_delete)
	{
		if(!is_somatic_)
		{
			report_config_->remove(VariantType::SVS, row);
		}
		else
		{
			som_report_config_->remove(VariantType::SVS, row);
			emit updateSomaticReportConfiguration();
		}
		updateReportConfigHeaderIcon(row);
	}
}

void SvWidget::openColumnSettings()
{
	SettingsDialog dlg(this);
	dlg.setWindowFlags(Qt::Window);
	dlg.gotoPage("columns", variantTypeToString(VariantType::SVS));
	if (dlg.exec()==QDialog::Accepted)
	{
		dlg.storeSettings();
	}
}

void SvWidget::adaptColumnWidthsAndHeights()
{
	ColumnConfig config = ColumnConfig::fromString(Settings::string("column_config_sv", true));
	config.applyColumnWidths(ui->svs);

	//chrA/chrB
	if (ui->svs->columnWidth(0)>50)
	{
		ui->svs->setColumnWidth(0, 50);
	}
	if (ui->svs->columnWidth(3)>50)
	{
		ui->svs->setColumnWidth(3, 50);
	}
}

void SvWidget::showAllColumns()
{
	for (int i=0; i<ui->svs->colorCount(); ++i)
	{
		ui->svs->setColumnHidden(i, false);
	}
}

void SvWidget::updateReportConfigHeaderIcon(int row)
{
	//report config-based filter is on => update whole variant list
	if (ui->filter_widget->reportConfigurationFilter()!=ReportConfigFilter::NONE)
	{
		applyFilters();
	}
	else //no filter => refresh icon only
	{
		QIcon report_icon;
		if (!is_somatic_ && report_config_->exists(VariantType::SVS, row))
		{
			const ReportVariantConfiguration& rc = report_config_->get(VariantType::SVS, row);
			report_icon = VariantTable::reportIcon(rc.showInReport(), rc.causal);
		}
		else if (is_somatic_ && som_report_config_->exists(VariantType::SVS, row))
		{
			const SomaticReportVariantConfiguration& rc = som_report_config_->get(VariantType::SVS, row);
			report_icon = VariantTable::reportIcon(rc.showInReport(), false);
		}
		ui->svs->verticalHeaderItem(row)->setIcon(report_icon);
	}
}

void SvWidget::editReportConfiguration(int row)
{
	if(!is_somatic_)
	{
		editGermlineReportConfiguration(row);
	}
	else
	{
		editSomaticReportConfiguration(row);
	}
}

void SvWidget::updateFormatAndInfoTables()
{
	QModelIndexList rows = ui->svs->selectionModel()->selectedRows();
	if(rows.count() != 1) return;

	int row = rows.at(0).row();

	//somatic
	if(svs_.format() == BedpeFileFormat::BEDPE_SOMATIC_TUMOR_NORMAL)
	{
		setSomaticInfos(row);
		return;
	}

	//determine format entries
	int i_format = svs_.annotationIndexByName("FORMAT");
	QByteArrayList format = svs_[row].annotations()[i_format].split(':');

	//get FORMAT data for samples
	QMap<QString, QMap<QByteArray, QByteArray>> data;
	foreach(QString ps, ps_names_)
	{
		int i_sample = svs_.annotationIndexByName(ps.toUtf8());
		data.insert(ps, svs_[row].getSampleFormatData(i_format, i_sample));
	}

	//get descriptionts
	QMap<QByteArray,QByteArray> format_description = svs_.metaInfoDescriptionByID("FORMAT");

	//set details
	ui->sv_details->setRowCount(format.count());
	for(int r=0; r<format.count(); ++r)
	{
		QByteArray key = format[r];
		ui->sv_details->setItem(r, 0, new QTableWidgetItem(QString(key)));
		ui->sv_details->setItem(r, 1, new QTableWidgetItem(QString(format_description.value(key))));
		for (int i=0; i<ps_names_.count(); ++i)
		{
			QString ps = ps_names_[i];
			ui->sv_details->setItem(r, 2 + i, new QTableWidgetItem(QString(data[ps].value(key))));
		}
	}
	resizeTableContent(ui->sv_details);
	ui->sv_details->scrollToTop();

	setInfoWidgets("INFO_A",row,ui->info_a);
	resizeTableContent(ui->info_a);
	setInfoWidgets("INFO_B",row,ui->info_b);
	resizeTableContent(ui->info_b);

	//size
	int size = svs_[row].size();
	ui->label_size->setText(size==-1 ? "" : "Size: " + QString::number(size));

	if (is_multisample_)
	{
		QVector<double> pe_af;
		QVector<double> sr_af;
		foreach (QString ps, ps_names_)
		{
			double af = alleleFrequency(row, ps.toUtf8(), "PR");
			if (af >= 0.0) pe_af.append(af);
			af = alleleFrequency(row, ps.toUtf8(), "SR");
			if (af >= 0.0) sr_af.append(af);
		}

		//Display Paired End Read AF of variant
		QString af_range;
		if (pe_af.size() > 0)
		{
			auto pe_minmax = std::minmax_element(pe_af.begin(), pe_af.end());
			af_range = QString::number(*pe_minmax.first, 'f',2) + "-" + QString::number(*pe_minmax.second, 'f',2);
		}
		ui->label_pe_af->setText("Paired End Read AF: " + af_range);

		//Display Split Read AF of variant
		af_range = "";
		if (sr_af.size() > 0)
		{
			auto sr_minmax = std::minmax_element(sr_af.begin(), sr_af.end());
			af_range = QString::number(*sr_minmax.first, 'f',2) + "-" + QString::number(*sr_minmax.second, 'f',2);
		}
		ui->label_sr_af->setText("Split Read AF: " + af_range);

	}
	else
	{
		QByteArray ps = ps_names_[0].toLatin1();
		ui->label_sr_af->setText("Split Read AF: " + QString::number(alleleFrequency(row, ps, "SR"), 'f',2));
		//Display Paired End Read AF of variant
		ui->label_pe_af->setText("Paired End Read AF: " + QString::number(alleleFrequency(row, ps, "PR"), 'f',2));
	}
}

void SvWidget::setInfoWidgets(const QByteArray &name, int row, QTableWidget* widget)
{
	//get info key-value pairs
	int i_info = svs_.annotationIndexByName(name);
	QByteArrayList raw_data = svs_[row].annotations()[i_info].split(';');
	if(raw_data.count() == 1 && (raw_data[0] == "." || raw_data[0] == "MISSING")) raw_data.clear();

	//get descriptions
	QMap<QByteArray,QByteArray> descriptions = svs_.metaInfoDescriptionByID("INFO");

	widget->setRowCount(raw_data.count());
	for(int i=0;i<raw_data.count();++i)
	{
		QByteArrayList tmp = raw_data[i].split('=');
		if(tmp.count()!=2) continue;

		widget->setItem(i, 0, new QTableWidgetItem(QString(tmp[0]))); // Key
		widget->setItem(i, 1, new QTableWidgetItem(QString(descriptions.value(tmp[0])))); //Description
		widget->setItem(i, 2, new QTableWidgetItem(QString(tmp[1]))); // Value
	}

	resizeTableContent(widget);
	widget->scrollToTop();
}


void SvWidget::setSomaticInfos(int row)
{
	//change titles for somatic info tables
	if (ui->title_sv_details->text() != "SV-Info:") ui->title_sv_details->setText("SV-Info:");
	if (ui->title_info_a->text() != "Breakpoint A:") ui->title_info_a->setText("Breakpoint A:");
	if (ui->title_info_b->text() != "Breakpoint B:") ui->title_info_b->setText("Breakpoint B:");

	//SV-Info table:

	ui->sv_details->setRowCount(5);

	QStringList names = {"TYPE", "SOMATICSCORE", "PR Evidence (ref/alt)", "SR Evidence (ref/alt)", "FLAGS"};
	QStringList descriptions = {"Type of structural variant", "Somatic variant quality score", "Paired read evidence for alt and ref allels", "Split read evidence for alt and ref allels", "Flags that occur in the info column"};

	for (int i=0; i < names.count(); i++)
	{
		QString name = names[i];
		QString desc = descriptions[i];

		if (name.contains("Evidence"))
		{
			QString evidence_type = "SR";
			if (name.contains("PR")) evidence_type = "PR";

			int col_idx_ref = GUIHelper::columnIndex(ui->svs, "TUM_" + evidence_type + "_REF", false);
			int col_idx_alt = GUIHelper::columnIndex(ui->svs, "TUM_" + evidence_type + "_ALT", false);

			if(col_idx_ref == -1 || col_idx_alt == -1)
			{
				QMessageBox::warning(this,"Error parsing annotation","Could not parse annotation for " + name);
			}
			else
			{
				QString text_ref = ui->svs->item(row, col_idx_ref)->text();
				QString text_alt = ui->svs->item(row, col_idx_alt)->text();
				ui->sv_details->setItem(i, 0, new QTableWidgetItem(name));
				ui->sv_details->setItem(i, 1, new QTableWidgetItem(desc));
				ui->sv_details->setItem(i, 2, new QTableWidgetItem(text_ref + " / " + text_alt));
			}
		}
		else
		{
			int col_idx = GUIHelper::columnIndex(ui->svs, name, false);
			if(col_idx == -1)
			{
				QMessageBox::warning(this,"Error parsing annotation","Could not parse annotation column TYPE");
			}
			else
			{
				QString text = ui->svs->item(row,col_idx)->text();
				ui->sv_details->setItem(i, 0, new QTableWidgetItem(name));
				ui->sv_details->setItem(i, 1, new QTableWidgetItem(desc));
				ui->sv_details->setItem(i, 2, new QTableWidgetItem(text));
			}
		}
	}
}

void SvWidget::showContextMenu(QPoint pos)
{
	QList<int> selected_rows = GUIHelper::selectedTableRows(ui->svs);

	//create menu
	QMenu menu(ui->svs);

	//special handling: publish comp-het sv combination
	if(selected_rows.count() == 2)
	{
		//ClinVar publication
		QAction* a_clinvar_pub = menu.addAction(QIcon("://Icons/ClinGen.png"), "Publish compound-heterozygote CNV in ClinVar");
        a_clinvar_pub->setEnabled(ngsd_user_logged_in_ && ! Settings::string("clinvar_api_key", true).trimmed().isEmpty());

		//execute menu
		QAction* action = menu.exec(ui->svs->viewport()->mapToGlobal(pos));
		if (action == a_clinvar_pub) uploadToClinvar(selected_rows.at(0), selected_rows.at(1));
		return;
	}
	if(selected_rows.count() != 1) return;

	int row = selected_rows.at(0);


	QAction* a_rep_edit = menu.addAction(QIcon(":/Icons/Report.png"), "Add/edit report configuration");
	a_rep_edit->setEnabled(rc_enabled_);
	QAction* a_rep_del = menu.addAction(QIcon(":/Icons/Remove.png"), "Delete report configuration");
	a_rep_del->setEnabled(false);

	if (is_somatic_)
	{
        a_rep_del->setEnabled((som_report_config_ != nullptr) && ngsd_user_logged_in_ && som_report_config_->exists(VariantType::SVS, row));
	}
	else
	{
		a_rep_del->setEnabled(rc_enabled_ && report_config_->exists(VariantType::SVS, row) && !report_config_->isFinalized());
	}

	menu.addSeparator();
	QAction* a_sv_val = menu.addAction("Perform structural variant validation");
    a_sv_val->setEnabled(ngsd_user_logged_in_);
	menu.addSeparator();
	QAction* a_ngsd_search = menu.addAction(QIcon(":/Icons/NGSD.png"), "Matching SVs in NGSD");
    a_ngsd_search->setEnabled(ngsd_user_logged_in_);
	menu.addSeparator();

	QAction* igv_pos1 = menu.addAction("Open position A in IGV");
	QAction* igv_pos2 = menu.addAction("Open position B in IGV");
	QAction* igv_split = menu.addAction("Open position A/B in IGV split screen");
	menu.addSeparator();
	QAction* copy_pos1 = menu.addAction("Copy position A to clipboard");
	QAction* copy_pos2 = menu.addAction("Copy position B to clipboard");
	menu.addSeparator();
	//ClinVar search
	QMenu* sub_menu = menu.addMenu(QIcon("://Icons/ClinGen.png"), "ClinVar");
	QAction* a_clinvar_find = sub_menu->addAction("Find in ClinVar");
	QAction* a_clinvar_pub = sub_menu->addAction("Publish in ClinVar");
    a_clinvar_pub->setEnabled(ngsd_user_logged_in_ && !Settings::string("clinvar_api_key", true).trimmed().isEmpty());
	//gene sub-menus
	int i_genes = svs_.annotationIndexByName("GENES", false);
	if (i_genes!=-1)
	{
		//use breakpoint genes only if the breakpoint genes column was right-clicked
		int column_index_clicked = ui->svs->columnAt(pos.x());
		QString column_name_clicked = ui->svs->horizontalHeaderItem(column_index_clicked)->text();
		if (column_name_clicked=="GENES_BREAKPOINTS")
		{
			i_genes = svs_.annotationIndexByName("GENES_BREAKPOINTS", false);
		}

		GeneSet genes = GeneSet::createFromText(svs_[row].annotations()[i_genes], ',');
		if (!genes.isEmpty())
		{
			menu.addSeparator();

			int gene_nr = 1;
			foreach(const QByteArray& gene, genes)
			{
				++gene_nr;
				if (gene_nr>=10) break; //don't show too many sub-menues for large variants!

				QMenu* sub_menu = menu.addMenu(gene);
                sub_menu->addAction(QIcon("://Icons/NGSD_gene.png"), "Gene tab")->setEnabled(ngsd_user_logged_in_);
				sub_menu->addAction(QIcon("://Icons/Google.png"), "Google");
				foreach(const GeneDB& db, GeneInfoDBs::all())
				{
					sub_menu->addAction(db.icon, db.name);
				}
			}
		}
	}

	//execute menu
	QAction* action = menu.exec(ui->svs->viewport()->mapToGlobal(pos));
	if (action == nullptr) return;

	//react
	QMenu* parent_menu = qobject_cast<QMenu*>(action->parent());
	const BedpeLine& sv = svs_[row];
	if (action==a_rep_edit)
	{
		editReportConfiguration(row);
	}
	else if (action==a_rep_del)
	{
		if(!is_somatic_)
		{
			report_config_->remove(VariantType::SVS, row);
		}
		else
		{
			som_report_config_->remove(VariantType::SVS, row);
			emit updateSomaticReportConfiguration();
		}
		updateReportConfigHeaderIcon(row);
	}
	else if (action==a_sv_val)
	{
		editSvValidation(row);
	}
	else if (action==a_ngsd_search)
	{
		SvSearchWidget* widget = new SvSearchWidget();
		widget->setProcessedSampleId(ps_id_);
		widget->setVariant(sv);
		auto dlg = GUIHelper::createDialog(widget, "SV search");

		dlg->exec();
	}
	else if (action==a_clinvar_find)
	{
		//get covert region
		BedFile regions = sv.affectedRegion();
		for (int i = 0; i < regions.count(); ++i)
		{
			const BedLine& region = regions[i];
			//create dummy variant to use GSvar helper
			Variant variant = Variant(region.chr(), region.start(), region.end(), "", "");
			QString url = GSvarHelper::clinVarSearchLink(variant, GSvarHelper::build());
			QDesktopServices::openUrl(QUrl(url));
		}
	}
	else if (action==a_clinvar_pub)
	{
		uploadToClinvar(row);
	}
	else if (action == igv_pos1)
	{
        IgvSessionManager::get(0).gotoInIGV(sv.position1(), true);
	}
	else if (action == igv_pos2)
	{
        IgvSessionManager::get(0).gotoInIGV(sv.position2(), true);
	}
	else if (action == igv_split)
	{
        IgvSessionManager::get(0).gotoInIGV(sv.position1() + " " + sv.position2(), true);
	}
	else if (action == copy_pos1)
	{
		QApplication::clipboard()->setText(sv.position1());
	}
	else if (action == copy_pos2)
	{
		QApplication::clipboard()->setText(sv.position2());
	}
	else if (parent_menu)
	{
		QString gene = parent_menu->title();
		QString db_name = action->text();

		if (db_name=="Gene tab")
		{
			GlobalServiceProvider::openGeneTab(gene);
		}
		else if (db_name=="Google")
		{
			QString query = gene + " AND (mutation";
            for (const Phenotype& pheno : ui->filter_widget->phenotypes())
			{
				query += " OR \"" + pheno.name() + "\"";
			}
			query += ")";

			QDesktopServices::openUrl(QUrl("https://www.google.com/search?q=" + query.replace("+", "%2B").replace(' ', '+')));
		}
		else //other databases
		{
			GeneInfoDBs::openUrl(db_name, gene);
		}
	}
}
