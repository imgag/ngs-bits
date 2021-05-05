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

SvWidget::SvWidget(const BedpeFile& bedpe_file, QString ps_id, FilterWidget* filter_widget, const GeneSet& het_hit_genes, QHash<QByteArray, BedFile>& cache, QWidget* parent, bool ini_gui)
	: QWidget(parent)
	, ui(new Ui::SvWidget)
	, sv_bedpe_file_(bedpe_file)
	, ps_ids_(QStringList())
	, ps_id_(ps_id)
	, variant_filter_widget_(filter_widget)
	, var_het_genes_(het_hit_genes)
	, gene2region_cache_(cache)
	, ngsd_enabled_(LoginManager::active())
	, report_config_(nullptr)
	, roi_gene_index_(roi_genes_)
{

	ui->setupUi(this);
	ui->svs->setContextMenuPolicy(Qt::CustomContextMenu);

	//link variant filter widget to FilterWidgetSV
	ui->filter_widget->setVariantFilterWidget(filter_widget);

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

	//clear GUI
	clearGUI();

	//init GUI
	if (ini_gui) initGUI();

	//set SV list type
	is_somatic_ = bedpe_file.isSomatic();

	//Disable filters that cannot apply for tumor normal pairs (data is expanded already)
	if(sv_bedpe_file_.format() == BedpeFileFormat::BEDPE_SOMATIC_TUMOR_NORMAL)
	{
		ui->info_a->setEnabled(false);
		ui->info_b->setEnabled(false);
		ui->sv_details->setEnabled(false);
	}
}

SvWidget::SvWidget(const BedpeFile& bedpe_file, QString ps_id, FilterWidget* filter_widget, QSharedPointer<ReportConfiguration> rep_conf, const GeneSet& het_hit_genes, QHash<QByteArray, BedFile>& cache, QWidget* parent)
    : SvWidget(bedpe_file, ps_id, filter_widget, het_hit_genes, cache, parent, false)
{
	if((bedpe_file.format()==BedpeFileFormat::BEDPE_GERMLINE_MULTI)||(bedpe_file.format()==BedpeFileFormat::BEDPE_GERMLINE_TRIO))
	{
		is_multisample_ = true;
		is_trio_ = bedpe_file.format()==BedpeFileFormat::BEDPE_GERMLINE_TRIO;
	}
	else if(bedpe_file.format()!=BedpeFileFormat::BEDPE_GERMLINE_SINGLE)
	{
		THROW(ProgrammingException, "Constructor in SvWidget has to be used using germline SV data.");
	}
	report_config_ = rep_conf;
	initGUI();
}

void SvWidget::initGUI()
{
	loading_svs_ = true;

	//clear GUI
	clearGUI();

	if(sv_bedpe_file_.count() == 0)
	{
		disableGUI("There are no SVs in the data file");
		loading_svs_ = false;
		return;
	}

	if(sv_bedpe_file_.format()==BedpeFileFormat::BEDPE_GERMLINE_MULTI || sv_bedpe_file_.format()==BedpeFileFormat::BEDPE_GERMLINE_TRIO)
    {
        // extract sample names from BEDPE file
        ps_names_.clear();
        foreach (const SampleInfo& sample_info, sv_bedpe_file_.sampleHeaderInfo())
        {
			ps_names_ << sample_info.id;
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
			if (is_trio_)
			{
				item->setToolTip((QStringList() << "child" << "father" << "mother").at(idx_sample));
			}
			else
			{
                item->setToolTip((sv_bedpe_file_.sampleHeaderInfo().at(idx_sample).isAffected())?"affected":"control");
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

	//Fill table widget with data from bedpe file
	for(int row=0; row<sv_bedpe_file_.count(); ++row)
	{
		//Set vertical header
		QTableWidgetItem* header_item = GUIHelper::createTableItem(QByteArray::number(row+1));
		if (report_variant_indices.contains(row))
		{
			bool show_report_icon = report_config_->get(VariantType::SVS, row).showInReport();
			header_item->setIcon(VariantTable::reportIcon(show_report_icon));
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
                QString gt = extractGenotype(sv_bedpe_file_[row], sv_bedpe_file_.annotationHeaders(), idx_sample);
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


	//only whole rows can be selected, one row at a time
	ui->svs->setSelectionBehavior(QAbstractItemView::SelectRows);
	resizeQTableWidget(ui->svs);

	loading_svs_ = false;

	//filter rows according default filter values
	applyFilters();
}


void SvWidget::clearGUI()
{
	ui->error_label->setHidden(true);
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
	GUIHelper::resizeTableCells(table_widget, 200, true, 100);
}


void SvWidget::applyFilters(bool debug_time)
{
	//skip if not necessary
	if (loading_svs_) return;
	int row_count = ui->svs->rowCount();
	if (row_count==0) return;

	FilterResult filter_result(row_count);

	// filters based on FilterWidgetSV

	try
	{
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
						//TODO > AXEL
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
						//TODO > AXEL
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
				if (!gene2region_cache_.contains(gene))
				{
					BedFile tmp = db.geneToRegions(gene, Transcript::ENSEMBL, "gene", true);
					tmp.clearAnnotations();
					tmp.extend(5000);
					tmp.merge();
					gene2region_cache_[gene] = tmp;
				}
				pheno_roi.add(gene2region_cache_[gene]);
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
	}
	catch(Exception& e)
	{
		QMessageBox::warning(this, "Filtering error", e.message() + "\nA possible reason for this error is an outdated variant list.\nTry re-annotating the NGSD columns.\n If re-annotation does not help, please re-analyze the sample (starting from annotation) in the sample information dialog!");

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

int SvWidget::colIndexbyName(const QString& name)
{
	for(int i=0;i<ui->svs->columnCount();++i)
	{
		if(ui->svs->horizontalHeaderItem(i)->text() == name) return i;
	}
	return -1;
}

int SvWidget::pairedEndReadCount(int row)
{
	int i_format =colIndexbyName("FORMAT");
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
	int i_format = colIndexbyName("FORMAT");
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
		QMessageBox::warning(this, "Error adding SV", "Structural varaints from special chromosomes cannot be imported into the NGSD!");
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
		QByteArrayList genes = sv_bedpe_file_[row].annotations()[i_genes].split(',');
		foreach(QByteArray gene, genes)
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

QByteArray SvWidget::extractGenotype(const BedpeLine& sv, const QList<QByteArray>& annotation_headers, int sample_idx)
{
    QByteArray genotype = sv.formatValueByKey("GT", annotation_headers, false, "FORMAT", sample_idx).trimmed();
    if (genotype == "1/1")
    {
        return "hom";
    }
    else if ((genotype == "0/1") || (genotype == "1/0"))
    {
        return "het";
    }
	else if (genotype == "0/0")
	{
		return "wt";
	}
    return "n/a";
}

void SvWidget::annotateTargetRegionGeneOverlap()
{
	QApplication::setOverrideCursor(Qt::BusyCursor);

	//generate gene regions and index
	BedFile roi_genes = NGSD().genesToRegions(ui->filter_widget->targetRegion().genes, Transcript::ENSEMBL, "gene", true);
	roi_genes.extend(5000);
	ChromosomalIndex<BedFile> roi_genes_index(roi_genes);

	// update gene tooltips
	int gene_idx = -1;
	for (int col_idx = 0; col_idx < ui->svs->columnCount(); ++col_idx)
	{
		if(ui->svs->horizontalHeaderItem(col_idx)->text().trimmed() == "GENES")
		{
			gene_idx = col_idx;
			break;
		}
	}

	if (gene_idx >= 0)
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
	int gene_idx = -1;
	for (int col_idx = 0; col_idx < ui->svs->columnCount(); ++col_idx)
	{
		if(ui->svs->horizontalHeaderItem(col_idx)->text().trimmed() == "GENES")
		{
			gene_idx = col_idx;
			break;
		}
	}
	if (gene_idx >= 0)
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
		emit openInIGV(coords);
	}
}

void SvWidget::disableGUI(const QString& message)
{
	setEnabled(false);

	ui->error_label->setHidden(false);
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
	if (!ngsd_enabled_) return;
	editReportConfiguration(row);
}

void SvWidget::svHeaderContextMenu(QPoint pos)
{
	if (!ngsd_enabled_ || (report_config_ == nullptr)) return;

	//skip somatic samples:
	if(is_somatic_) return;

	//get variant index
	int row = ui->svs->verticalHeader()->visualIndexAt(pos.ry());

	//set up menu
	QMenu menu(ui->svs->verticalHeader());
	QAction* a_edit = menu.addAction(QIcon(":/Icons/Report.png"), "Add/edit report configuration");
	QAction* a_delete = menu.addAction(QIcon(":/Icons/Remove.png"), "Delete report configuration");
	a_delete->setEnabled(!is_somatic_ && !report_config_->isFinalized() && report_config_->exists(VariantType::SVS, row));

	//exec menu
	pos = ui->svs->verticalHeader()->viewport()->mapToGlobal(pos);
	QAction* action = menu.exec(pos);
	if (action==nullptr) return;

	if(!LoginManager::active()) return; //do nothing if no access to NGSD

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
			updateReportConfigHeaderIcon(row);
		}
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
			report_icon = VariantTable::reportIcon(report_config_->get(VariantType::SVS, row).showInReport());
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
}

void SvWidget::SvSelectionChanged()
{
	if(sv_bedpe_file_.format() == BedpeFileFormat::BEDPE_SOMATIC_TUMOR_NORMAL) return; //Skip somatic lists because info columns are expanded already

	QModelIndexList rows = ui->svs->selectionModel()->selectedRows();
	if(rows.count() != 1) return;

	int row = rows.at(0).row();

	int i_format = colIndexbyName("FORMAT");

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
		ui->sv_details->setItem(i,1,new QTableWidgetItem(QString(format_description.value(format.at(i).toLatin1()))));
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
	int i_info = colIndexbyName(name);
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

void SvWidget::showContextMenu(QPoint pos)
{
	QModelIndexList rows = ui->svs->selectionModel()->selectedRows();
	if(rows.count() != 1) return;

	int row = rows.at(0).row();

	//create menu
	QMenu menu(ui->svs);
	QAction* a_rep_edit = menu.addAction(QIcon(":/Icons/Report.png"), "Add/edit report configuration");
	a_rep_edit->setEnabled((report_config_ != nullptr) && ngsd_enabled_ && !is_somatic_);
	QAction* a_rep_del = menu.addAction(QIcon(":/Icons/Remove.png"), "Delete report configuration");
	a_rep_del->setEnabled(false);
	a_rep_del->setEnabled((report_config_ != nullptr) && ngsd_enabled_ && !is_somatic_ && report_config_->exists(VariantType::SVS, row) && !report_config_->isFinalized());
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
	//gene sub-menus
	int i_genes = sv_bedpe_file_.annotationIndexByName("GENES", false);
	if (i_genes!=-1)
	{
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
				sub_menu->addAction(QIcon("://Icons/NGSD_gene.png"), "Gene tab")->setEnabled(LoginManager::active());
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
		widget->setCoordinates(sv);
		auto dlg = GUIHelper::createDialog(widget, "SV search");

		dlg->exec();
	}
	else if (action == igv_pos1)
	{
		emit(openInIGV(sv.position1()));
	}
	else if (action == igv_pos2)
	{
		emit(openInIGV(sv.position2()));
	}
	else if (action == igv_split)
	{
		emit(openInIGV(sv.position1() + " " + sv.position2()));
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
			openGeneTab(gene);
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
