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


MosaicWidget::MosaicWidget(const VariantList& variants, QString ps_id, ReportSettings rep_settings, QHash<QByteArray, BedFile>& cache, QWidget* parent)
	: QWidget(parent)
	, ui_(new Ui::MosaicWidget)
	, variants_(variants)
	, filter_result_()
	, report_settings_()
	, gene2region_cache_(cache)
{
	ui_->setupUi(this);

	connect(ui_->filter_widget, SIGNAL(filtersChanged()), this, SLOT(applyFilters()));
	connect(ui_->mosaics,SIGNAL(itemDoubleClicked(QTableWidgetItem*)),this,SLOT(variantDoubleClicked(QTableWidgetItem*)));
	ui_->filter_widget->setValidFilterEntries(variants_.filters().keys());
	report_settings_ = rep_settings;
	initGUI();
}

void MosaicWidget::initGUI()
{
	//set up GUI
	updateGUI();

	//apply filters
	applyFilters();
}

MosaicWidget::~MosaicWidget()
{
	delete ui_;
}


void MosaicWidget::variantDoubleClicked(QTableWidgetItem *item)
{
	if (item==nullptr) return;

	int row = item->row();

	const Variant& v = variants_[ui_->mosaics->rowToVariantIndex(row)];
	GlobalServiceProvider::gotoInIGV(v.chr().str() + ":" + QString::number(v.start()) + "-" + QString::number(v.end()), true);
}


void MosaicWidget::applyFilters(bool debug_time)
{
	const int rows = variants_.count();

	try
	{
		QTime timer;
		timer.start();

		//apply main filter
		const FilterCascade& filter_cascade = ui_->filter_widget->filters();
		filter_result_ = filter_cascade.apply(variants_, false, debug_time);
		ui_->filter_widget->markFailedFilters();

		if (debug_time)
		{
			Log::perf("Applying annotation filters took ", timer);
			timer.start();
		}

		//filter by report config
		ReportConfigFilter rc_filter = ui_->filter_widget->reportConfigurationFilter();
		if (rc_filter!=ReportConfigFilter::NONE)
		{
			for(int r=0; r<rows; ++r)
			{
				if (!filter_result_.flags()[r]) continue;

				if (rc_filter==ReportConfigFilter::HAS_RC)
				{

					filter_result_.flags()[r] = report_settings_.report_config->exists(VariantType::CNVS, r);

				}
				else if (rc_filter==ReportConfigFilter::NO_RC)
				{

					filter_result_.flags()[r] = !report_settings_.report_config->exists(VariantType::CNVS, r);

				}
			}
		}

		//filter by genes
		GeneSet genes = ui_->filter_widget->genes();
		if (!genes.isEmpty())
		{
			FilterGenes filter;
			filter.setStringList("genes", genes.toStringList());
			filter.apply(variants_, filter_result_);

			if (debug_time)
			{
				Log::perf("Applying gene filter took ", timer);
				timer.start();
			}
		}

		//filter by ROI
		if (ui_->filter_widget->targetRegion().isValid())
		{
			for(int r=0; r<rows; ++r)
			{
				if (!filter_result_.flags()[r]) continue;

				filter_result_.flags()[r] = ui_->filter_widget->targetRegion().regions.overlapsWith(variants_[r].chr(), variants_[r].start(), variants_[r].end());
			}
		}

		//filter by region
		QString region_text = ui_->filter_widget->region();
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
				if (!filter_result_.flags()[r]) continue;

				filter_result_.flags()[r] = region.overlapsWith(variants_[r].chr(), variants_[r].start(), variants_[r].end());
			}
		}

		//filter by phenotype (via genes, not genomic regions)
		PhenotypeList phenotypes = ui_->filter_widget->phenotypes();
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
				if (!filter_result_.flags()[r]) continue;

				filter_result_.flags()[r] = pheno_roi.overlapsWith(variants_[r].chr(), variants_[r].start(), variants_[r].end());
			}
		}

		//filter annotations by text
		QByteArray text = ui_->filter_widget->text().trimmed().toLower();
		if (text!="")
		{
			for(int r=0; r<rows; ++r)
			{
				if (!filter_result_.flags()[r]) continue;

				bool match = false;
				foreach(const QByteArray& anno, variants_[r].annotations())
				{
					if (anno.toLower().contains(text))
					{
						match = true;
						break;
					}
				}
				filter_result_.flags()[r] = match;
			}
		}

	}
	catch(Exception& e)
	{
		QMessageBox::warning(this, "Filtering error", e.message() + "\nA possible reason for this error is an outdated variant list.\nTry re-annotating the NGSD columns.\n If re-annotation does not help, please re-analyze the sample (starting from annotation) in the sample information dialog!");

		filter_result_ = FilterResult(variants_.count(), false);
	}

	//update GUI
	for(int r=0; r<rows; ++r)
	{
		ui_->mosaics->setRowHidden(r, !filter_result_.flags()[r]);
	}
	updateStatus(filter_result_.countPassing());
}

void MosaicWidget::updateGUI(bool keep_widths)
{
	QApplication::setOverrideCursor(Qt::BusyCursor);

	QTime timer;
	timer.start();

	//apply filters
	applyFilters();
	int passing_variants = filter_result_.countPassing();
	QString status = QString::number(passing_variants) + " of " + QString::number(variants_.count()) + " variants passed filters.";
	int max_variants = 10000;
	if (passing_variants>max_variants)
	{
		status += " Displaying " + QString::number(max_variants) + " variants only!";
	}
	ui_->status->setText(status);

	Log::perf("Applying all filters took ", timer);
	timer.start();

	//update variant table
	QList<int> col_widths = ui_->mosaics->columnWidths();
	AnalysisType type = variants_.type();

	if (type==GERMLINE_SINGLESAMPLE || type==GERMLINE_TRIO || type==GERMLINE_MULTISAMPLE)
	{
		ui_->mosaics->update(variants_, filter_result_, report_settings_, max_variants);
	}
	else
	{
		THROW(ProgrammingException, "Unsupported analysis type in refreshVariantTable!");
	}

	ui_->mosaics->adaptRowHeights();
	if (keep_widths)
	{
		ui_->mosaics->setColumnWidths(col_widths);
	}
	else
	{
		ui_->mosaics->adaptColumnWidths();
	}
	QApplication::restoreOverrideCursor();

}

void MosaicWidget::copyToClipboard()
{
	GUIHelper::copyToClipboard(ui_->mosaics);
}

void MosaicWidget::copyVariantsToClipboard()
{
	GUIHelper::copyToClipboard(ui_->mosaics, true);
}


void MosaicWidget::updateStatus(int shown)
{
	QString text = QString::number(shown) + "/" + QString::number(variants_.count()) + " passing filter(s)";
	ui_->status->setText(text);
}

