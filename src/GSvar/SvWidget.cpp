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

SvWidget::SvWidget(QWidget* parent, const BedpeFile& bedpe_file, QString ps_id, QSharedPointer<ReportConfiguration> rep_conf, const GeneSet& het_hit_genes)
	: QWidget(parent)
	, ui(new Ui::SvWidget)
	, sv_bedpe_file_(bedpe_file)
	, ps_ids_(QStringList())
	, ps_id_(ps_id)
	, var_het_genes_(het_hit_genes)
	, report_config_(rep_conf)
	, ngsd_enabled_(LoginManager::active())
	, rc_enabled_(ngsd_enabled_ && (report_config_!=nullptr && !report_config_->isFinalized()))
	, roi_gene_index_(roi_genes_)
	, is_somatic_(sv_bedpe_file_.isSomatic())
	, is_multisample_(bedpe_file.format()==BedpeFileFormat::BEDPE_GERMLINE_TRIO || bedpe_file.format()==BedpeFileFormat::BEDPE_GERMLINE_MULTI)
{
	ui->setupUi(this);

	GUIHelper::styleSplitter(ui->splitter);
	ui->splitter->setStretchFactor(0, 10);
	ui->splitter->setStretchFactor(1, 1);

	//Setup signals and slots
	connect(ui->copy_to_clipboard,SIGNAL(clicked()),this,SLOT(copyToClipboard()));
	connect(ui->svs,SIGNAL(itemDoubleClicked(QTableWidgetItem*)),this,SLOT(SvDoubleClicked(QTableWidgetItem*)));
	connect(ui->svs,SIGNAL(itemSelectionChanged()),this,SLOT(SvSelectionChanged()));
	connect(ui->svs,SIGNAL(customContextMenuRequested(QPoint)),this,SLOT(showContextMenu(QPoint)));
	connect(ui->filter_widget, SIGNAL(filtersChanged()), this, SLOT(applyFilters()));
	connect(ui->filter_widget, SIGNAL(phenotypeImportNGSDRequested()), this, SLOT(importPhenotypesFromNGSD()));
	connect(ui->filter_widget, SIGNAL(targetRegionChanged()), this, SLOT(clearTooltips()));
	connect(ui->filter_widget, SIGNAL(calculateGeneTargetRegionOverlap()), this, SLOT(annotateTargetRegionGeneOverlap()));
	connect(ui->svs->verticalHeader(), SIGNAL(sectionDoubleClicked(int)), this, SLOT(svHeaderDoubleClicked(int)));
	ui->svs->verticalHeader()->setContextMenuPolicy(Qt::CustomContextMenu);
	connect(ui->svs->verticalHeader(), SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(svHeaderContextMenu(QPoint)));
	connect(ui->resize_btn, SIGNAL(clicked(bool)), this, SLOT(adaptColumnWidthsCustom()));

	//clear GUI
	clearGUI();

	initGUI();
}

SvWidget::~SvWidget()
{
	delete ui;
}

SvWidget::SvWidget(QWidget* parent, const BedpeFile& bedpe_file, QString ps_id, SomaticReportConfiguration& som_rep_conf, const GeneSet& het_hit_genes)
	: QWidget(parent)
	, ui(new Ui::SvWidget)
	, sv_bedpe_file_(bedpe_file)
	, ps_ids_(QStringList())
	, ps_id_(ps_id)
	, var_het_genes_(het_hit_genes)
	, som_report_config_(&som_rep_conf)
	, ngsd_enabled_(LoginManager::active())
	, rc_enabled_(ngsd_enabled_)
	, roi_gene_index_(roi_genes_)
	, is_somatic_(sv_bedpe_file_.isSomatic())
	, is_multisample_(bedpe_file.format()==BedpeFileFormat::BEDPE_GERMLINE_TRIO || bedpe_file.format()==BedpeFileFormat::BEDPE_GERMLINE_MULTI)
{
	if(! bedpe_file.isSomatic())
	{
		THROW(ProgrammingException, "SvWidget constructor for somatic Analysis was used with a BEDPE file that is not from a somatic analysis.");
	}

	ui->setupUi(this);

	GUIHelper::styleSplitter(ui->splitter);
	ui->splitter->setStretchFactor(0, 10);
	ui->splitter->setStretchFactor(1, 1);

	//Setup signals and slots
	connect(ui->copy_to_clipboard,SIGNAL(clicked()),this,SLOT(copyToClipboard()));
	connect(ui->svs,SIGNAL(itemDoubleClicked(QTableWidgetItem*)),this,SLOT(SvDoubleClicked(QTableWidgetItem*)));
	connect(ui->svs,SIGNAL(itemSelectionChanged()),this,SLOT(SvSelectionChanged()));
	connect(ui->svs,SIGNAL(customContextMenuRequested(QPoint)),this,SLOT(showContextMenu(QPoint)));
	connect(ui->filter_widget, SIGNAL(filtersChanged()), this, SLOT(applyFilters()));
	connect(ui->filter_widget, SIGNAL(phenotypeImportNGSDRequested()), this, SLOT(importPhenotypesFromNGSD()));
	connect(ui->filter_widget, SIGNAL(targetRegionChanged()), this, SLOT(clearTooltips()));
	connect(ui->filter_widget, SIGNAL(calculateGeneTargetRegionOverlap()), this, SLOT(annotateTargetRegionGeneOverlap()));
	connect(ui->svs->verticalHeader(), SIGNAL(sectionDoubleClicked(int)), this, SLOT(svHeaderDoubleClicked(int)));
	ui->svs->verticalHeader()->setContextMenuPolicy(Qt::CustomContextMenu);
	connect(ui->svs->verticalHeader(), SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(svHeaderContextMenu(QPoint)));
	connect(ui->resize_btn, SIGNAL(clicked(bool)), this, SLOT(adaptColumnWidthsCustom()));


	//clear GUI
	clearGUI();

	//Disable filters that cannot apply for tumor normal pairs (data is expanded already)
	if(sv_bedpe_file_.format() == BedpeFileFormat::BEDPE_SOMATIC_TUMOR_NORMAL)
	{
		ui->info_a->setEnabled(false);
		ui->info_b->setEnabled(false);
		ui->sv_details->setEnabled(false);
	}

	initGUI();
}

void SvWidget::initGUI()
{
	//clear GUI
	clearGUI();

	if(sv_bedpe_file_.count() == 0)
	{
		disableGUI("There are no SVs in the data file");
		return;
	}

	if(sv_bedpe_file_.format()==BedpeFileFormat::BEDPE_GERMLINE_MULTI || sv_bedpe_file_.format()==BedpeFileFormat::BEDPE_GERMLINE_TRIO)
	{
		// extract sample names from BEDPE file
		ps_names_.clear();
		foreach (const SampleInfo& sample_info, sv_bedpe_file_.sampleHeaderInfo())
		{
			ps_names_ << sample_info.name;
		}

		// get processed sample ids from NGSD
		if (ngsd_enabled_)
		{
			ps_ids_.clear();
			ps_id_ = "";
			foreach (QString ps_name, ps_names_)
			{
				ps_ids_ << NGSD().processedSampleId(ps_name);
			}
		}
	}
    else
    {
        // single sample
		ps_ids_.clear();
		if(ps_id_ != "" && ngsd_enabled_)
        {
			ps_names_ = QStringList() << NGSD().processedSampleName(ps_id_);
        }
        else
        {
			// fallback: use empty sample header
			ps_names_ = QStringList() << "";
        }
	}

	//Set list of annotations to be showed, by default some annotations are filtered out
	QByteArrayList annotation_headers = sv_bedpe_file_.annotationHeaders();

	for(int i=0;i<annotation_headers.count();++i)
	{
		if(annotation_headers[i].contains("STRAND_")) continue;
		if(annotation_headers[i].contains("NAME_")) continue;
		if(annotation_headers[i] == "ID") continue;
		annotations_to_show_ << annotation_headers[i];
	}

	//add genotype of samples as separate column for trio/multisample after the positions
	if (is_multisample_)
	{
		int col_idx = ui->svs->columnCount();
		ui->svs->setColumnCount(ui->svs->columnCount() + ps_names_.size());

		for (int idx_sample = 0; idx_sample < ps_names_.size(); ++idx_sample)
		{
			QTableWidgetItem* item;
			item = new QTableWidgetItem(QString(ps_names_.at(idx_sample)));
			if (sv_bedpe_file_.format()==BedpeFileFormat::BEDPE_GERMLINE_TRIO)
			{
				item->setToolTip((QStringList() << "child" << "father" << "mother").at(idx_sample));
			}
			else
			{
				item->setToolTip(sv_bedpe_file_.sampleHeaderInfo().at(idx_sample).isAffected() ? "affected" : "control");
			}
			if (sv_bedpe_file_.sampleHeaderInfo().at(idx_sample).isAffected()) item->setForeground(QBrush(Qt::darkRed));

			ui->svs->setHorizontalHeaderItem(col_idx + idx_sample, item);
		}
	}


	//Add annotation headers
	QList<int> annotation_indices;
	for(int i=0;i<sv_bedpe_file_.annotationHeaders().count();++i)
	{
		QByteArray header = sv_bedpe_file_.annotationHeaders()[i];

		if(!annotations_to_show_.contains(header)) continue;

		ui->svs->setColumnCount(ui->svs->columnCount() + 1 );
        QTableWidgetItem* item = new QTableWidgetItem(QString(header));
		if (header=="OMIM")
		{
			item->setIcon(QIcon("://Icons/Table.png"));
			item->setToolTip("Double click table cell to open table view of annotations");
		}

		if(sv_bedpe_file_.annotationDescriptionByName(header) != "")
		{
			item->setToolTip(sv_bedpe_file_.annotationDescriptionByName(header));
		}

		ui->svs->setHorizontalHeaderItem(ui->svs->columnCount() - 1, item);
		annotation_indices << i;
	}

	//Fill rows
	ui->svs->setRowCount(sv_bedpe_file_.count());

	//Get report variant indices
	QSet<int> report_variant_indices;
	if((report_config_ != NULL) && !is_somatic_) report_variant_indices = report_config_->variantIndices(VariantType::SVS, false).toSet();
	if((som_report_config_ != NULL) && is_somatic_) report_variant_indices = som_report_config_->variantIndices(VariantType::SVS, false).toSet();

	//TODO move to a "germlineOverviewTables()" function
	//Fill table widget with data from bedpe file
	for(int row=0; row<sv_bedpe_file_.count(); ++row)
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

		//Fill fixed columns
		ui->svs->setItem(row,0,new QTableWidgetItem(QString(sv_bedpe_file_[row].chr1().str())));
		ui->svs->setItem(row,1,new QTableWidgetItem(QString::number(sv_bedpe_file_[row].start1())));
		ui->svs->setItem(row,2,new QTableWidgetItem(QString::number(sv_bedpe_file_[row].end1())));

		ui->svs->setItem(row,3,new QTableWidgetItem(QString(sv_bedpe_file_[row].chr2().str())));
		ui->svs->setItem(row,4,new QTableWidgetItem(QString::number(sv_bedpe_file_[row].start2())));
		ui->svs->setItem(row,5,new QTableWidgetItem(QString::number(sv_bedpe_file_[row].end2())));

		int col_in_widget = 6;

        //add genotype of samples as separate column for trio/multisample after the positions
        if (is_multisample_)
        {
            for (int idx_sample = 0; idx_sample < ps_names_.size(); ++idx_sample)
            {
				QString gt = sv_bedpe_file_[row].genotypeHumanReadable(sv_bedpe_file_.annotationHeaders(), false, idx_sample);
                ui->svs->setItem(row, col_in_widget++, new QTableWidgetItem(gt));
            }
        }

		//Fill annotation columns
		foreach(int anno_index,annotation_indices)
		{
			ui->svs->setItem(row,col_in_widget,new QTableWidgetItem(QString(sv_bedpe_file_[row].annotations().at(anno_index))));
			++col_in_widget;
		}
	}

	// TODO move to multisample function:
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
	foreach (const QByteArray& entry, sv_bedpe_file_.metaInfoDescriptionByID("FILTER").keys())
	{
		valid_filter_entries.append(entry);
	}
	valid_filter_entries.removeAll("PASS");
	valid_filter_entries.prepend("PASS");
	ui->filter_widget->setValidFilterEntries(valid_filter_entries);


	//only whole rows can be selected, but multiple rows at a time
	ui->svs->setSelectionBehavior(QAbstractItemView::SelectRows);
	ui->svs->setSelectionMode(QAbstractItemView::ExtendedSelection);
	resizeQTableWidget(ui->svs);

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

void SvWidget::resizeQTableWidget(QTableWidget *table_widget)
{
	table_widget->resizeRowToContents(0);

	int height = table_widget->rowHeight(0);

	for (int i=0; i<table_widget->rowCount(); ++i)
	{
		table_widget->setRowHeight(i, height);
	}
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

		QTime timer;
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
		filter_result = filter_cascade.apply(sv_bedpe_file_, false, debug_time);
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

				if (!sv_bedpe_file_[row].intersectsWith(ui->filter_widget->targetRegion().regions, true)) filter_result.flags()[row] = false;
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

				const BedpeLine& sv = sv_bedpe_file_[row];
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
			foreach(const Phenotype& pheno, phenotypes)
			{
				pheno_genes << db.phenotypeToGenes(db.phenotypeIdByAccession(pheno.accession()), true);
			}

			//convert genes to ROI (using a cache to speed up repeating queries)
			BedFile pheno_roi;
			foreach(const QByteArray& gene, pheno_genes)
			{
				pheno_roi.add(GlobalServiceProvider::geneToRegions(gene, db));
			}
			pheno_roi.merge();

			for(int row=0; row<row_count; ++row)
			{
				if (!filter_result.flags()[row]) continue;

				filter_result.flags()[row] = sv_bedpe_file_[row].intersectsWith(pheno_roi, true);
			}
		}

		//filter by genes
		GeneSet gene_whitelist = ui->filter_widget->genes();
		if (!gene_whitelist.isEmpty())
		{
			QByteArray genes_joined = gene_whitelist.join('|');

			// get column index of 'GENES' column
			int i_genes = sv_bedpe_file_.annotationIndexByName("GENES", false);
			if (i_genes == -1)
			{
				QMessageBox::warning(this, "Filtering error", "BEDPE files does not contain a 'GENES' column! \nFiltering based on genes is not possible. Please reannotate the structural variant file.");
			}
			else
			{
				if (genes_joined.contains("*")) //with wildcards
				{
					QRegExp reg(genes_joined.replace("-", "\\-").replace("*", "[A-Z0-9-]*"));
					for(int row=0; row<row_count; ++row)
					{
						if (!filter_result.flags()[row]) continue;

						// generate GeneSet from column text
						GeneSet sv_genes = GeneSet::createFromText(sv_bedpe_file_[row].annotations()[i_genes], ',');

						bool match_found = false;
						foreach(const QByteArray& sv_gene, sv_genes)
						{
							if (reg.exactMatch(sv_gene))
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
						GeneSet sv_genes = GeneSet::createFromText(sv_bedpe_file_[row].annotations()[i_genes], ',');

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
				foreach(const QByteArray& anno, sv_bedpe_file_[row].annotations())
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

int SvWidget::pairedEndReadCount(int row)
{
	int i_format = GUIHelper::columnIndex(ui->svs, "FORMAT", false);
	if(i_format == -1) return -1;

	QByteArray desc = ui->svs->item(row,i_format)->text().toUtf8();
	QByteArray data = ui->svs->item(row,i_format+1)->text().toUtf8();

	QByteArrayList infos = getFormatEntryByKey("PR",desc,data).split(',');
	if(infos.count() != 2) return -1;
	//return paired end read alteration count
	bool success = false ;

	int pe_reads = infos[1].trimmed().toInt(&success);
	if(!success) return -1;
	return pe_reads;
}


double SvWidget::alleleFrequency(int row, const QByteArray& read_type, int sample_idx)
{
	int i_format = GUIHelper::columnIndex(ui->svs, "FORMAT", false);
	if(i_format == -1 ) return -1.;

	if(ui->svs->columnCount() < i_format+1) return -1.;

	QByteArray desc = ui->svs->item(row,i_format)->text().toUtf8();
	QByteArray data = ui->svs->item(row,i_format+1+sample_idx)->text().toUtf8();

	int count_ref = 0;
	int count_alt = 0;

	QByteArrayList pr;
	if(read_type == "PR") pr = getFormatEntryByKey("PR",desc,data).split(',');
	else if(read_type == "SR") pr =  getFormatEntryByKey("SR",desc,data).split(',');
	else return -1;

	if(pr.count() != 2) return -1;

	bool success = false;
	count_ref = pr[0].toInt(&success);
	if(!success) return -1;
	count_alt = pr[1].toInt(&success);
	if(!success) return -1;

	if(count_alt+count_ref != 0) return (double)count_alt / (count_alt+count_ref);
	else return 0;
}

void SvWidget::editSvValidation(int row)
{
	BedpeLine& sv = sv_bedpe_file_[row];

	try
	{
		NGSD db;

		//get SV ID
		QString callset_id = db.getValue("SELECT id FROM sv_callset WHERE processed_sample_id=:0", true, ps_id_).toString();
		if (callset_id == "") THROW(DatabaseException, "No callset found for processed sample id " + ps_id_ + "!");
		QString sv_id = db.svId(sv, Helper::toInt(callset_id, "callset_id"), sv_bedpe_file_);
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

	if(!sv_bedpe_file_[row].chr1().isNonSpecial() || !sv_bedpe_file_[row].chr2().isNonSpecial())
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
	int i_genes = sv_bedpe_file_.annotationIndexByName("genes", false);
	if (i_genes!=-1)
	{
		GeneSet genes = GeneSet::createFromText(sv_bedpe_file_[row].annotations()[i_genes], ',');
		foreach(const QByteArray& gene, genes)
		{

			GeneInfo gene_info = db.geneInfo(gene);
			inheritance_by_gene << KeyValuePair{gene, gene_info.inheritance};
		}
	}

	//exec dialog
	ReportVariantDialog dlg(sv_bedpe_file_[row].toString(), inheritance_by_gene, var_config, this);
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

	if(!sv_bedpe_file_[row].chr1().isNonSpecial() || !sv_bedpe_file_[row].chr2().isNonSpecial())
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
	SomaticReportVariantDialog dlg(sv_bedpe_file_[row].toString(), var_config, this);
	dlg.setEnabled(true);
	if (dlg.exec()!=QDialog::Accepted) return;

	//update config, GUI and NGSD
	som_report_config_->addSomaticVariantConfiguration(var_config);
	emit updateSomaticReportConfiguration();
	updateReportConfigHeaderIcon(row);
}

void SvWidget::uploadToClinvar(int index1, int index2)
{
	if (!ngsd_enabled_) return;
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
		data.sv1 = sv_bedpe_file_[index1];
		if(data.sv1.type() == StructuralVariantType::BND)
			WARNING(NotImplementedException, "The upload of translocations is not supported by the ClinVar API. Please use the manual submission through the ClinVar website.");

		if(data.submission_type == ClinvarSubmissionType::CompoundHeterozygous)
		{
			data.sv2 = sv_bedpe_file_[index2];
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
		data.genes = data.sv1.genes(sv_bedpe_file_.annotationHeaders());
		if(data.submission_type == ClinvarSubmissionType::CompoundHeterozygous) data.genes <<  data.sv2.genes(sv_bedpe_file_.annotationHeaders());

		//get callset id
		QString callset_id = db.getValue("SELECT id FROM sv_callset WHERE processed_sample_id=:0", true, ps_id_).toString();
		if (callset_id == "") THROW(DatabaseException, "No callset found for processed sample id " + ps_id_ + "!");

		//determine NGSD ids of variant and report variant for variant 1
		QString sv_id = db.svId(data.sv1, callset_id.toInt(), sv_bedpe_file_, false);
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
			sv_id = db.svId(data.sv2, callset_id.toInt(), sv_bedpe_file_, true);
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
			BedFile sv_regions = sv_bedpe_file_[row_idx].affectedRegion();
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


void SvWidget::SvDoubleClicked(QTableWidgetItem *item)
{
	if (item==nullptr) return;

	int col = item->column();
	int row = item->row();

	QString col_name = item->tableWidget()->horizontalHeaderItem(col)->text();
	if (col_name=="OMIM")
	{
		int omim_index = sv_bedpe_file_.annotationHeaders().indexOf("OMIM");

		QString text = sv_bedpe_file_[row].annotations()[omim_index].trimmed();
		if (text.trimmed()!="")
		{
			VariantDetailsDockWidget::showOverviewTable("OMIM entries of ROH " +  sv_bedpe_file_[row].toString(), text, ';', "https://www.omim.org/entry/");
		}
	}
	else
	{
		QString coords = sv_bedpe_file_[row].positionRange();
        IgvSessionManager::get(0).gotoInIGV(coords, true);
	}
}

void SvWidget::disableGUI(const QString& message)
{
	setEnabled(false);

	ui->error_label->setText("<font color='red'>" + message + "</font>");
}

QByteArray SvWidget::getFormatEntryByKey(const QByteArray& key, const QByteArray &format_desc, const QByteArray &format_data)
{
	QByteArray key_tmp = key.trimmed();

	QByteArrayList descs = format_desc.split(':');
	QByteArrayList datas = format_data.split(':');
	if(descs.count() != datas.count()) return "";

	for(int i=0;i<descs.count();++i)
	{
		if(descs[i] == key_tmp)
		{
			return datas[i];
		}
	}

	return "";
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
	if (!ngsd_enabled_ || (report_config_ == nullptr && som_report_config_ == nullptr)) return;
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

void SvWidget::SvSelectionChanged()
{
	QModelIndexList rows = ui->svs->selectionModel()->selectedRows();
	if(rows.count() != 1) return;

	int row = rows.at(0).row();

	if(sv_bedpe_file_.format() == BedpeFileFormat::BEDPE_SOMATIC_TUMOR_NORMAL)
	{
		//display somatic specific infos:
		setSomaticInfos(row);
		return;
	}

	//display germline infos: expanded INFO fields
	int i_format = GUIHelper::columnIndex(ui->svs, "FORMAT", false);

	// adapt for multisample
	int i_format_first_data = i_format+1;
	//Check whether col with format data actually exists
	if(ui->svs->columnCount()-1 < i_format_first_data ) return;

	QStringList format = ui->svs->item(row,i_format)->text().split(":");
	QVector<QStringList> data;
	for(int sample_idx=0; sample_idx < ps_names_.size(); sample_idx++)
	{
		data.append(ui->svs->item(row,i_format_first_data + sample_idx)->text().split(":"));
		if(format.count() != data.last().count()) return;
	}
	ui->sv_details->setRowCount(format.count());

	//Map with description of format field, e.g. GT <-> GENOTYPE
	QMap<QByteArray,QByteArray> format_description = sv_bedpe_file_.metaInfoDescriptionByID("FORMAT");

	for(int i=0;i<ui->sv_details->rowCount();++i)
	{
		ui->sv_details->setItem(i,0,new QTableWidgetItem(QString(format[i])));
		ui->sv_details->setItem(i,1,new QTableWidgetItem(QString(format_description.value(format.at(i).toUtf8()))));
		for(int sample_idx=0; sample_idx < ps_names_.size(); sample_idx++)
		{
			ui->sv_details->setItem(i,2 + sample_idx, new QTableWidgetItem(QString(data.at(sample_idx).at(i))));
		}

	}
	resizeQTableWidget(ui->sv_details);
	ui->sv_details->scrollToTop();

	setInfoWidgets("INFO_A",row,ui->info_a);
	resizeQTableWidget(ui->info_a);
	setInfoWidgets("INFO_B",row,ui->info_b);
	resizeQTableWidget(ui->info_b);

	//size
	int size = sv_bedpe_file_[row].size();
	ui->label_size->setText(size==-1 ? "" : "Size: " + QString::number(size));

	if (sv_bedpe_file_.format() == BedpeFileFormat::BEDPE_GERMLINE_MULTI || sv_bedpe_file_.format() == BedpeFileFormat::BEDPE_GERMLINE_TRIO)
	{
		QVector<double> pe_af;
		QVector<double> sr_af;
		for (int i = 0; i < sv_bedpe_file_.sampleHeaderInfo().size(); ++i)
		{
			double af = alleleFrequency(row, "PR", i);
			if (af >= 0.0) pe_af.append(af);
			af = alleleFrequency(row, "SR", i);
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
		//Display Split Read AF of variant
		ui->label_sr_af->setText("Split Read AF: " + QString::number(alleleFrequency(row, "SR"), 'f',2));
		//Display Paired End Read AF of variant
		ui->label_pe_af->setText("Paired End Read AF: " + QString::number(alleleFrequency(row, "PR"), 'f',2));
	}
}

void SvWidget::setInfoWidgets(const QByteArray &name, int row, QTableWidget* widget)
{
	int i_info = GUIHelper::columnIndex(ui->svs, name, false);
	if(i_info == -1)
	{
		QMessageBox::warning(this,"Error parsing annotation","Could not parse annotation column " + name);
		return;
	}

	QStringList raw_data = ui->svs->item(row,i_info)->text().split(";");

	if(raw_data.count() > 1) widget->setRowCount(raw_data.count());
	else if(raw_data.count() == 1 && (raw_data[0] == "." || raw_data[0] == "MISSING")) widget->setRowCount(0);
	else widget->setRowCount(raw_data.count());

	QMap<QByteArray,QByteArray> descriptions = sv_bedpe_file_.metaInfoDescriptionByID("INFO");

	for(int i=0;i<raw_data.count();++i)
	{
		QStringList tmp = raw_data[i].split("=");
		if(tmp.count() != 2 ) continue;

		widget->setItem(i,0,new QTableWidgetItem(QString(tmp[0]))); // Key

		widget->setItem(i,1, new QTableWidgetItem(QString(descriptions.value(tmp[0].toUtf8())))); //Description
		widget->setItem(i,2,new QTableWidgetItem(QString(tmp[1]))); // Value
	}

	resizeQTableWidget(widget);
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

	// Breakpoint A:


	// Breakpoint B:



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
		a_clinvar_pub->setEnabled(ngsd_enabled_ && ! Settings::string("clinvar_api_key", true).trimmed().isEmpty());

		//execute menu
		QAction* action = menu.exec(ui->svs->viewport()->mapToGlobal(pos));
		if (action == a_clinvar_pub) uploadToClinvar(selected_rows.at(0), selected_rows.at(1));
		return;
	}
	if(selected_rows.count() != 1) return;

	int row = selected_rows.at(0);


	QAction* a_rep_edit = menu.addAction(QIcon(":/Icons/Report.png"), "Add/edit report configuration");
	a_rep_edit->setEnabled(rc_enabled_ || (som_report_config_ != nullptr && ngsd_enabled_));
	QAction* a_rep_del = menu.addAction(QIcon(":/Icons/Remove.png"), "Delete report configuration");
	a_rep_del->setEnabled(false);

	if (is_somatic_)
	{
		a_rep_del->setEnabled((som_report_config_ != nullptr) && ngsd_enabled_ && som_report_config_->exists(VariantType::SVS, row));
	}
	else
	{
		a_rep_del->setEnabled(rc_enabled_ && report_config_->exists(VariantType::SVS, row) && !report_config_->isFinalized());
	}

	menu.addSeparator();
	QAction* a_sv_val = menu.addAction("Perform structural variant validation");
	a_sv_val->setEnabled(ngsd_enabled_);
	menu.addSeparator();
	QAction* a_ngsd_search = menu.addAction(QIcon(":/Icons/NGSD.png"), "Matching SVs in NGSD");
	a_ngsd_search->setEnabled(ngsd_enabled_);
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
	a_clinvar_pub->setEnabled(ngsd_enabled_ && !Settings::string("clinvar_api_key", true).trimmed().isEmpty());
	//gene sub-menus
	int i_genes = sv_bedpe_file_.annotationIndexByName("GENES", false);
	if (i_genes!=-1)
	{
		//use breakpoint genes only if the breakpoint genes column was right-clicked
		int column_index_clicked = ui->svs->columnAt(pos.x());
		QString column_name_clicked = ui->svs->horizontalHeaderItem(column_index_clicked)->text();
		if (column_name_clicked=="GENES_BREAKPOINTS")
		{
			i_genes = sv_bedpe_file_.annotationIndexByName("GENES_BREAKPOINTS", false);
		}

		GeneSet genes = GeneSet::createFromText(sv_bedpe_file_[row].annotations()[i_genes], ',');
		if (!genes.isEmpty())
		{
			menu.addSeparator();

			int gene_nr = 1;
			foreach(const QByteArray& gene, genes)
			{
				++gene_nr;
				if (gene_nr>=10) break; //don't show too many sub-menues for large variants!

				QMenu* sub_menu = menu.addMenu(gene);
				sub_menu->addAction(QIcon("://Icons/NGSD_gene.png"), "Gene tab")->setEnabled(ngsd_enabled_);
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
	const BedpeLine& sv = sv_bedpe_file_[row];
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
			foreach(const Phenotype& pheno, ui->filter_widget->phenotypes())
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

void SvWidget::adaptColumnWidthsCustom()
{
	try
	{
		int idx = GUIHelper::columnIndex(ui->svs, "FILTER");
		ui->svs->setColumnWidth(idx, 45);

		idx = GUIHelper::columnIndex(ui->svs, "REF_A");
		ui->svs->setColumnWidth(idx, 45);

		idx = GUIHelper::columnIndex(ui->svs, "ALT_A");
		ui->svs->setColumnWidth(idx, 45);

		idx = GUIHelper::columnIndex(ui->svs, "REF_B");
		ui->svs->setColumnWidth(idx, 45);

		idx = GUIHelper::columnIndex(ui->svs, "ALT_B");
		ui->svs->setColumnWidth(idx, 45);

		idx = GUIHelper::columnIndex(ui->svs, "INFO_A");
		ui->svs->setColumnWidth(idx, 45);

		idx = GUIHelper::columnIndex(ui->svs, "INFO_B");
		ui->svs->setColumnWidth(idx, 45);

		idx = GUIHelper::columnIndex(ui->svs, "FORMAT");
		ui->svs->setColumnWidth(idx, 45);

		idx = GUIHelper::columnIndex(ui->svs, "GENE_INFO");
		ui->svs->setColumnWidth(idx, 45);

		idx = GUIHelper::columnIndex(ui->svs, "NGSD_HOM");
		ui->svs->setColumnWidth(idx, 45);

		idx = GUIHelper::columnIndex(ui->svs, "NGSD_HET");
		ui->svs->setColumnWidth(idx, 45);

		idx = GUIHelper::columnIndex(ui->svs, "NGSD_AF");
		ui->svs->setColumnWidth(idx, 45);

		idx = GUIHelper::columnIndex(ui->svs, "NGSD_SV_BREAKPOINT_DENSITY");
		ui->svs->setColumnWidth(idx, 85);

		idx = GUIHelper::columnIndex(ui->svs, "OMIM");
		ui->svs->setColumnWidth(idx, 750);
	}
	catch(Exception& e)
	{
		QMessageBox::warning(this, "Column adjustment error", "Error while adjusting column widths:\n" + e.message());
	}
}
