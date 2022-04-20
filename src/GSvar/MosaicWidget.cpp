#include "MosaicWidget.h"
#include "Helper.h"
#include "Exceptions.h"
#include "GUIHelper.h"
#include "GSvarHelper.h"
#include "VariantDetailsDockWidget.h"
#include "NGSD.h"
#include "Settings.h"
#include "Log.h"
#include "ProcessedSampleWidget.h"
#include "Histogram.h"
#include "ReportVariantDialog.h"
#include "LoginManager.h"
#include "GeneInfoDBs.h"
#include "ValidationDialog.h"
#include "GlobalServiceProvider.h"
#include <QMessageBox>
#include <QFileInfo>
#include <QBitArray>
#include <QClipboard>
#include <QMenu>
#include <QDesktopServices>
#include <QUrl>
#include <QDir>
#include <QInputDialog>
#include <QChartView>
QT_CHARTS_USE_NAMESPACE


MosaicWidget::MosaicWidget(const VariantList& variants, QString ps_id, FilterWidget* filter_widget, QSharedPointer<ReportConfiguration> rep_conf, const GeneSet& het_hit_genes, QHash<QByteArray, BedFile>& cache, QWidget* parent)
	: QWidget(parent)
	, ui(new Ui::MosaicWidget)
	, ps_id_(ps_id)
	, variants_(variants)
	, report_config_()
	, var_het_genes_(het_hit_genes)
	, gene2region_cache_(cache)
	, ngsd_enabled_(LoginManager::active())
{
	ui->setupUi(this);

	//set small variant filters TODO remove ?
//	ui->filter_widget->setVariantFilterWidget(filter_widget);

	report_config_ = rep_conf;
	initGUI();
}

void MosaicWidget::initGUI()
{
	//set up GUI
	try
	{
//		updateGUI();
	}
	catch(Exception e)
	{
//		addInfoLine("<font color='red'>Error parsing file:\n" + e.message() + "</font>");
//		disableGUI();
	}

	//apply filters
	applyFilters();
}

MosaicWidget::~MosaicWidget()
{
	delete ui;
}


void MosaicWidget::applyFilters(bool debug_time)
{
	const int rows = variants_.count();
	FilterResult filter_result(rows);

	try
	{
		QTime timer;
		timer.start();

		//apply main filter
		const FilterCascade& filter_cascade = ui->filter_widget->filters();
		//set comp-het gene list the first time the filter is applied
		for(int i=0; i<filter_cascade.count(); ++i)
		{
			const FilterCnvCompHet* comphet_filter = dynamic_cast<const FilterCnvCompHet*>(filter_cascade[i].data());
			if (comphet_filter!=nullptr && comphet_filter->hetHitGenes().count()!=var_het_genes_.count())
			{
				comphet_filter->setHetHitGenes(var_het_genes_);
			}
		}
		filter_result = filter_cascade.apply(variants_, false, debug_time);
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
			for(int r=0; r<rows; ++r)
			{
				if (!filter_result.flags()[r]) continue;

				if (rc_filter==ReportConfigFilter::HAS_RC)
				{

					filter_result.flags()[r] = report_config_->exists(VariantType::CNVS, r);

				}
				else if (rc_filter==ReportConfigFilter::NO_RC)
				{

					filter_result.flags()[r] = !report_config_->exists(VariantType::CNVS, r);

				}
			}
		}

		//filter by genes
		GeneSet genes = ui->filter_widget->genes();
		if (!genes.isEmpty())
		{
			QByteArray genes_joined = genes.join('|');
			//TODO
		}

		//filter by ROI
		if (ui->filter_widget->targetRegion().isValid())
		{
			for(int r=0; r<rows; ++r)
			{
				if (!filter_result.flags()[r]) continue;

				filter_result.flags()[r] = ui->filter_widget->targetRegion().regions.overlapsWith(variants_[r].chr(), variants_[r].start(), variants_[r].end());
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
			for(int r=0; r<rows; ++r)
			{
				if (!filter_result.flags()[r]) continue;

				filter_result.flags()[r] = region.overlapsWith(variants_[r].chr(), variants_[r].start(), variants_[r].end());
			}
		}

		//filter by phenotype (via genes, not genomic regions)
		PhenotypeList phenotypes = ui->filter_widget->phenotypes();
		if (!phenotypes.isEmpty())
		{
			//convert phenotypes to genes
			NGSD db;
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

			for(int r=0; r<rows; ++r)
			{
				if (!filter_result.flags()[r]) continue;

				filter_result.flags()[r] = pheno_roi.overlapsWith(variants_[r].chr(), variants_[r].start(), variants_[r].end());
			}
		}

		//filter annotations by text
		QByteArray text = ui->filter_widget->text().trimmed().toLower();
		if (text!="")
		{
			for(int r=0; r<rows; ++r)
			{
				if (!filter_result.flags()[r]) continue;

				bool match = false;
				foreach(const QByteArray& anno, variants_[r].annotations())
				{
					if (anno.toLower().contains(text))
					{
						match = true;
						break;
					}
				}
				filter_result.flags()[r] = match;
			}
		}

	}
	catch(Exception& e)
	{
		QMessageBox::warning(this, "Filtering error", e.message() + "\nA possible reason for this error is an outdated variant list.\nTry re-annotating the NGSD columns.\n If re-annotation does not help, please re-analyze the sample (starting from annotation) in the sample information dialog!");

		filter_result = FilterResult(variants_.count(), false);
	}

	//update GUI
	for(int r=0; r<rows; ++r)
	{
		ui->mosaics->setRowHidden(r, !filter_result.flags()[r]);
	}
	updateStatus(filter_result.countPassing());
}

void MosaicWidget::copyToClipboard()
{
	GUIHelper::copyToClipboard(ui->mosaics);
}


void MosaicWidget::updateStatus(int shown)
{
	QString text = QString::number(shown) + "/" + QString::number(variants_.count()) + " passing filter(s)";
	ui->status->setText(text);
}

