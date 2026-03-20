#include "FilterWidget.h"
#include "Settings.h"
#include "Helper.h"
#include "NGSD.h"
#include "ScrollableTextDialog.h"
#include "PhenotypeSelectionWidget.h"
#include "PhenotypeSettingsDialog.h"
#include "GUIHelper.h"
#include "GSvarHelper.h"
#include "DBSelector.h"
#include <QFileInfo>
#include <QFileDialog>
#include <QCompleter>
#include <QMenu>
#include <QMessageBox>
#include "LoginManager.h"
#include "IgvSessionManager.h"
#include <QSortFilterProxyModel>
#include <QClipboard>
#include "SubpanelDesignDialog.h"
#include "FilterWidgetHelper.h"


FilterWidget::FilterWidget(QWidget *parent)
	: QWidget(parent)
	, ui_()
    , data_controller_(AnalysisDataController::instance())
    , state_(AnalysisDataController::instance().getSmallVariantsFilterState())
{
	ui_.setupUi(this);
	ui_.cascade_widget->setSubject(VariantType::SNVS_INDELS);

	connect(ui_.cascade_widget, SIGNAL(filterCascadeChanged()), this, SLOT(updateFilterName()));
    connect(ui_.cascade_widget, SIGNAL(filterCascadeChanged()), this, SLOT(updateFilterCascade()));
	connect(ui_.cascade_widget, SIGNAL(customFilterLoaded()), this, SLOT(customFilterLoaded()));
	connect(ui_.filters, SIGNAL(currentIndexChanged(int)), this, SLOT(setFilter(int)));
	ui_.lab_modified->setHidden(true);

	connect(ui_.roi_add, SIGNAL(clicked()), this, SLOT(addRoi()));
	connect(ui_.roi_add_temp, SIGNAL(clicked()), this, SLOT(addRoiTemp()));
	connect(ui_.roi_remove, SIGNAL(clicked()), this, SLOT(removeRoi()));
    connect(ui_.roi, SIGNAL(currentIndexChanged(int)), this, SLOT(updateTargetRegionFilter(int)));
    connect(&state_, SIGNAL(targetRegionChanged(const TargetRegionInfo&)), this, SLOT(updateGeneWarning()));

    connect(ui_.gene, SIGNAL(editingFinished()), this, SLOT(updateGeneFilter()));
    connect(ui_.text, SIGNAL(editingFinished()), this, SLOT(updateTextFilter()));
    connect(ui_.region, SIGNAL(editingFinished()), this, SLOT(updateRegionFilter()));
    connect(ui_.report_config, SIGNAL(currentIndexChanged(int)), this, SLOT(updateReportConfigfilter()));

	connect(ui_.roi, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(showRoiContextMenu(QPoint)));

	connect(ui_.hpo_terms, SIGNAL(clicked(QPoint)), this, SLOT(editPhenotypes()));
	connect(ui_.hpo_terms, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(showPhenotypeContextMenu(QPoint)));

	connect(ui_.gene, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(showGeneContextMenu(QPoint)));

	ui_.clearn_btn->setMenu(new QMenu());
	ui_.clearn_btn->menu()->addAction("Clear filters", this, SLOT(clearFilters()));
	ui_.clearn_btn->menu()->addAction("Clear filters and ROI", this, SLOT(clearFiltersAndRoi()));

	ui_.roi_btn->setMenu(new QMenu());
	ui_.roi_btn->menu()->addAction(QIcon(":/Icons/Info.png"), "Show information", this, SLOT(showTargetRegionDetails()));
	ui_.roi_btn->menu()->addAction(QIcon(":/Icons/Clipboard.png"),"Copy target region to clipboard", this, SLOT(copyTargetRegionToClipboard()));
	ui_.roi_btn->menu()->addAction(QIcon(":/Icons/Clipboard.png"),"Copy genes to clipboard", this, SLOT(copyGenesToClipboard()));
	ui_.roi_btn->menu()->addAction(QIcon(":/Icons/IGV.png"),"Open target region in IGV", this, SLOT(openTargetRegionInIGV()));


    // connect changes in state to the gui
    connect(&state_, SIGNAL(filterNameChanged(const QString&)), this, SLOT(updateFilterName()));
    connect(&state_, SIGNAL(filterCascadeChanged(const FilterCascade&)), this, SLOT(updateFilterCascade()));
    connect(&state_, SIGNAL(targetRegionChanged(const TargetRegionInfo&)), this, SLOT(updateTargetRegionFilter(const TargetRegionInfo&)));
    connect(&state_, SIGNAL(genesChanged(const GeneSet&)), this, SLOT(updateFilterName()));
    connect(&state_, SIGNAL(regionFilterChanged(const BedLine&)), this, SLOT(updateRegionFilter()));
    connect(&state_, SIGNAL(phenotypesChanged(const PhenotypeList&)), this, SLOT(phenotypesChanged()));
    connect(&state_, SIGNAL(reportConfigFilterChanged(const ReportConfigFilter&)), this, SLOT(updateReportConfigfilter()));

	try
	{
		loadFilters();
	}
	catch(Exception& e)
	{
		QMessageBox::warning(this, "Filter load failed", "Filter file could not be opened:\n" + e.message());
	}
	reset(true);
}

void FilterWidget::setValidFilterEntries(const QStringList& filter_entries)
{
	ui_.cascade_widget->setValidFilterEntries(filter_entries);
}

void FilterWidget::markFailedFilters()
{
	ui_.cascade_widget->markFailedFilters();
}

void FilterWidget::loadTargetRegions()
{
    FilterWidgetHelper::loadTargetRegions(ui_.roi);
}

void FilterWidget::resetSignalsUnblocked(bool clear_roi)
{
	//filter cols
	ui_.cascade_widget->clear();
	ui_.filters->setCurrentIndex(0);

	//rois
	if (clear_roi)
	{
		ui_.roi->setCurrentIndex(1);
		ui_.gene_warning->setHidden(true);
	}

	//gene
	ui_.gene->clear();
	ui_.text->clear();
	ui_.region->clear();
	ui_.report_config->setCurrentIndex(0);

	//phenotype
    updatePhenotypes();
}

QString FilterWidget::filterFileName()
{
	return GSvarHelper::applicationBaseName() + "_filters.ini";
}

bool FilterWidget::setFilter(QString name)
{
	for (int i=0; i<ui_.filters->count(); ++i)
	{
		if (ui_.filters->itemText(i)==name)
		{
			ui_.filters->setCurrentIndex(i);
			return true;
		}
	}

	return false;
}

void FilterWidget::setFilterCascade(const FilterCascade& filter_cascade)
{
	ui_.cascade_widget->setFilters(filter_cascade);
}

void FilterWidget::editColumnFilter(QString col)
{
	ui_.cascade_widget->editColumnFilter(col);
}

QString FilterWidget::filterName() const
{
	return ui_.filters->currentText();
}

void FilterWidget::updateNGSDSupport()
{
	ui_.hpo_terms->setEnabled(LoginManager::active());
}

void FilterWidget::reset(bool clear_roi)
{
	blockSignals(true);
	resetSignalsUnblocked(clear_roi);
	blockSignals(false);
}


QString FilterWidget::targetRegionDisplayName() const
{
	return ui_.roi->currentText();
}

QComboBox* FilterWidget::targetRegionBox()
{
    return ui_.roi;
}

bool FilterWidget::setTargetRegionByName(QString name)
{
    return FilterWidgetHelper::setTargetRegionByName(name, ui_.roi);
}


void FilterWidget::setReportConfigFilter(const ReportConfigFilter& rc_config)
{
    if (rc_config == ReportConfigFilter::NONE)
    {
        ui_.report_config->setCurrentIndex(0);
        ui_.report_config->setEnabled(false);
    }
    else if (rc_config == ReportConfigFilter::HAS_RC)
    {
        ui_.report_config->setCurrentIndex(1);
    }
    else if (rc_config == ReportConfigFilter::NO_RC)
    {
        ui_.report_config->setCurrentIndex(2);
    }
    else
    {
        THROW(ArgumentException, "Unhandeled ReportConfigFilter type!");
    }
}

void FilterWidget::updateFilterName()
{
    if (ui_.filters->currentText()=="[none]")
    {
        state_.setFilterName("", false);
    }
    else
    {
        state_.setFilterName(ui_.filters->currentText(), false);
    }

    ui_.lab_modified->setHidden(false);
}

void FilterWidget::updateFilterCascade()
{
    state_.setFilterCascade(ui_.cascade_widget->filters(), false);
}

void FilterWidget::updateTargetRegionFilter(int index)
{
    //delete old completer
    QCompleter* completer_old = ui_.roi->completer();
    if (completer_old!=nullptr)
    {
        completer_old->deleteLater();
    }

    //create completer for search mode
    if (ui_.roi->currentIndex()==0)
    {
        ui_.roi->setEditable(true);

        QSortFilterProxyModel *proxy_model = new QSortFilterProxyModel(ui_.roi);
        proxy_model->setSourceModel(ui_.roi->model());
        proxy_model->setFilterCaseSensitivity(Qt::CaseInsensitive);

        QCompleter *completer = new QCompleter(proxy_model, ui_.roi);
        completer->setCompletionMode(QCompleter::PopupCompletion);
        completer->setFilterMode(Qt::MatchContains);
        completer->setCaseSensitivity(Qt::CaseInsensitive);
        completer->setCompletionRole(Qt::DisplayRole);
        ui_.roi->setCompleter(completer);
    }
    else
    {
        ui_.roi->setEditable(false);
    }

    //load target region data
    QString roi_name = ui_.roi->itemData(index).toString().trimmed();
    try
    {
        state_.setTargetRegionInfoByName(roi_name, false);
    }
    catch(Exception& e)
    {
        QMessageBox::warning(this, "Error loading target region '" + state_.getTargetRegionInfo().name + "'", e.message());
        clearTargetRegion();
    }
}

void FilterWidget::updateTargetRegionFilter(const TargetRegionInfo& new_target)
{
    FilterWidgetHelper::setTargetRegionByName(new_target.name, ui_.roi);
}

void FilterWidget::updateRegionFilter()
{
    BedLine region_filter = BedLine::fromString(ui_.region->displayText());
    if (!region_filter.isValid()) //check if valid chr
    {
        Chromosome chr(ui_.region->displayText());
        if (chr.isNonSpecial())
        {
            region_filter.setChr(chr);
            region_filter.setStart(1);
            region_filter.setEnd(999999999);
        }
    }

    if (region_filter.isValid()) state_.setRegionFilter(region_filter, false);
}

void FilterWidget::updateGeneFilter()
{
    state_.setGenes(GeneSet::createFromText(ui_.gene->displayText().toUtf8(), ','), false);
}

void FilterWidget::updateTextFilter()
{
    state_.setTextFilter(ui_.text->displayText(), false);
}

void FilterWidget::updateReportConfigfilter()
{
    if (ui_.report_config->currentText() == "n/a")
    {
        state_.setReportConfigFilter(ReportConfigFilter::NONE, false);
    }
    else if (ui_.report_config->currentText().contains("with"))
    {
        state_.setReportConfigFilter(ReportConfigFilter::HAS_RC, false);
    }
    else  if (ui_.report_config->currentText().contains("without"))
    {
        state_.setReportConfigFilter(ReportConfigFilter::NO_RC, false);
    }
    else
    {
        THROW(ArgumentException, "Value in report config QComboBox coundn't be translated to a ReportConfigFilter type");
    }
}


void FilterWidget::addRoi()
{
	//get file to open
	QString path = Settings::path("path_regions", true);
	QString filename = QFileDialog::getOpenFileName(this, "Select target region file", path, "BED files (*.bed);;All files (*.*)");
	if (filename=="") return;

	//store open path
	Settings::setPath("path_regions", filename);

	//update settings
	QStringList rois = Settings::stringList("target_regions", true);
	rois.append(filename);
	rois.sort(Qt::CaseInsensitive);
	rois.removeDuplicates();
	Settings::setStringList("target_regions", rois);

	//update GUI
	loadTargetRegions();
}

void FilterWidget::addRoiTemp()
{
	//get file to open
	QString path = Settings::path("path_regions", true);
	QString filename = QFileDialog::getOpenFileName(this, "Select target region file", path, "BED files (*.bed);;All files (*.*)");
	if (filename=="") return;

	//add to list
	ui_.roi->addItem(QFileInfo(filename).fileName(), Helper::canonicalPath(filename));
}

void FilterWidget::removeRoi()
{
	QString filename = ui_.roi->itemData(ui_.roi->currentIndex()).toString();
	if (filename=="") return;

	//update settings
	QStringList rois = Settings::stringList("target_regions", true);
	rois.removeOne(filename);
	Settings::setStringList("target_regions", rois);

	//update GUI
	loadTargetRegions();
}

void FilterWidget::updatePhenotypes()
{
	//update phenotype history
    FilterWidgetHelper::updatePhenotypeHistory(state_.getPhenotypes());

	//update phenotype list
	QByteArrayList tmp;
    foreach (const Phenotype& pheno, state_.getPhenotypes())
	{
		tmp << pheno.name();
	}
	ui_.hpo_terms->setText(tmp.join("; "));

	//update tooltip
	QString tooltip = "Phenotype filter based on HPO terms.<br><br>Notes:<br>- This functionality is only available when NGSD is enabled.<br>- Filters based on phenotype-associated gene loci including 5000 flanking bases.";
    if (!state_.getPhenotypes().isEmpty())
	{
		tooltip += "<br><br><nobr>Selected HPO terms:</nobr>";
        foreach (const Phenotype& pheno, state_.getPhenotypes())
		{
			tooltip += "<br><nobr>" + pheno.toString() + "</nobr>";
		}

		tooltip += "<br><br><nobr>Selected phenotype-gene sources:</nobr>";
		tooltip += "<br><nobr>";
        foreach (const PhenotypeSource& s, state_.getPhenotypeSettings().sources)
		{
			tooltip += Phenotype::sourceToString(s) + ", ";
		}
		tooltip.chop(2);
		tooltip += "</nobr>";

		tooltip += "<br><br><nobr>Selected phenotype-gene evidence levels:</nobr>";
		tooltip += "<br><nobr>";
        foreach(const PhenotypeEvidenceLevel& e, state_.getPhenotypeSettings().evidence_levels)
		{
			tooltip += Phenotype::evidenceToString(e) + ", ";
		}
		tooltip.chop(2);
		tooltip += "</nobr>";

		tooltip += "<br><br><nobr>Selected phenotype combination mode:</nobr>";
        tooltip += QString("<br>") + (state_.getPhenotypeSettings().mode==PhenotypeCombimnationMode::MERGE ? "merge" : "intersect");
	}
	ui_.hpo_terms->setToolTip(tooltip);

	//show icon if settings are changed
	static QAction* settings_action = new QAction(QIcon(":/Icons/settings.png"), "");
    if (state_.getPhenotypeSettings()!=PhenotypeSettings())
	{
		ui_.hpo_terms->addAction(settings_action, QLineEdit::TrailingPosition);
	}
	else
	{
		ui_.hpo_terms->removeAction(settings_action);
	}
}

void FilterWidget::customFilterLoaded()
{
	ui_.filters->blockSignals(true);
	ui_.filters->setCurrentIndex(0);
	ui_.filters->blockSignals(false);

	ui_.lab_modified->setHidden(false);
}

void FilterWidget::showTargetRegionDetails()
{
    const TargetRegionInfo& roi = state_.getTargetRegionInfo();
    if (!roi.isValid()) return;

	//ROI statistics
	QStringList text;
    text << "Target region: " + roi.name;
    text << "Regions: " + QString::number(roi.regions.count());
    text << "Bases: " + QString::number(roi.regions.baseCount());
	text << "";

    if (roi.genes.count()!=0)
	{
        text << "Genes: " + QString::number(roi.genes.count());
        text << roi.genes.join(", ");
	}
	else
	{
		text << "Genes: gene file not available for this target region!";
	}

	//show text
	ScrollableTextDialog dlg(this, "Target region details");
	dlg.appendLines(text);
	dlg.exec();
}

void FilterWidget::copyGenesToClipboard()
{
    QApplication::clipboard()->setText(state_.getTargetRegionInfo().genes.toString("\n"));
}

void FilterWidget::copyTargetRegionToClipboard()
{
    QApplication::clipboard()->setText(state_.getTargetRegionInfo().regions.toText());
}

void FilterWidget::openTargetRegionInIGV()
{

    QString roi_file = GSvarHelper::localRoiFolder() + state_.getTargetRegionInfo().name + ".bed";
    state_.getTargetRegionInfo().regions.store(roi_file);

    IgvSessionManager::get(0).loadFileInIGV(roi_file, false);
}

void FilterWidget::updateGeneWarning()
{
	QStringList warnings;

	//indikationsspezifische Abrechnung
    if (state_.getTargetRegionInfo().isValid() && !state_.getTargetRegionInfo().genes.isEmpty())
	{
        QString warning = GSvarHelper::specialGenes(state_.getTargetRegionInfo().genes);
		if (!warning.isEmpty())
		{
			warnings.append(warning);
		}
	}

	ui_.gene_warning->setToolTip(warnings.join("\n\n"));
	ui_.gene_warning->setHidden(warnings.isEmpty());
}

void FilterWidget::editPhenotypes()
{
	//edit
	PhenotypeSelectionWidget* selector = new PhenotypeSelectionWidget(this);
    selector->setPhenotypes(state_.getPhenotypes());

	auto dlg = GUIHelper::createDialog(selector, "Select HPO terms", "", true);

	//update
	if (dlg->exec()==QDialog::Accepted)
	{
        state_.setPhenotypes(selector->selectedPhenotypes(), false);
	}
}

void FilterWidget::showPhenotypeContextMenu(QPoint pos)
{
	//set up
	QMenu menu;
	QAction* a_load = menu.addAction(QIcon(":/Icons/NGSD_sample.png"), "load from sample");
	a_load->setEnabled(LoginManager::active());
	QMenu* history_menu = menu.addMenu("history");
    foreach(const PhenotypeList& entry, FilterWidgetHelper::phenotypeHistory())
	{
		history_menu->addAction(entry.toString());
	}
	QAction* a_clear = menu.addAction(QIcon(":/Icons/Trash.png"), "clear");
	menu.addSeparator();
	QAction* a_subpanel = LoginManager::active() ? menu.addAction("create sub-panel") : nullptr;
	menu.addSeparator();
	QAction* a_settings = menu.addAction(QIcon(":/Icons/settings.png"), "settings");

	//exec
	QAction* action = menu.exec(ui_.hpo_terms->mapToGlobal(pos));
	if (action==nullptr) return;

	if (action==a_clear)
	{
        state_.clearPhenotypeFilter();
	}
	else if (action==a_load)
	{
        QString ps_name = data_controller_.germlineReportSupported() ? data_controller_.germlineReportSample() : data_controller_.getMainSampleName();
        NGSD db;
        QString sample_id = db.sampleId(ps_name);
        PhenotypeList phenotypes = db.getSampleData(sample_id).phenotypes;

        state_.setPhenotypes(phenotypes, false);
	}
	else if (action==a_subpanel)
	{
        FilterWidgetHelper::createSubPanelFromPhenotypeFilter(state_.getPhenotypes(), ui_.roi, state_);
	}
	else if (action==a_settings)
	{
		PhenotypeSettingsDialog dlg(this);
        dlg.set(state_.getPhenotypeSettings());

		//update
		if (dlg.exec()==QDialog::Accepted)
		{
            state_.setPhenotypeSettings(dlg.get(), false);
		}
	}
	else if (action->parent()==history_menu)
	{
        foreach(const PhenotypeList& entry, FilterWidgetHelper::phenotypeHistory())
		{
            if (action->text()==entry.toString()) state_.setPhenotypes(entry, false);
		}
	}
}

void FilterWidget::showGeneContextMenu(QPoint pos)
{
	//set up
	QMenu menu;
	if (LoginManager::active())
	{
		menu.addAction("select via disease");
	}
	if (!ui_.gene->text().trimmed().isEmpty())
	{
		menu.addAction(QIcon(":/Icons/Trash.png"), "clear");
	}

	//exec
	QAction* action = menu.exec(ui_.gene->mapToGlobal(pos));
	if (action==nullptr) return;

	if (action->text()=="clear")
	{
		ui_.gene->setText("");
        updateGeneFilter();
	}
	else if (action->text()=="select via disease")
	{
		//create
		NGSD db;
		DBSelector* selector = new DBSelector(this);
		selector->fill(db.createTable("disease_term", "SELECT id, name FROM disease_term ORDER BY name ASC"), true);

		//execute
		QSharedPointer<QDialog> dialog = GUIHelper::createDialog(selector, "Select disease", "Disease:", true);
		if (dialog->exec()==QDialog::Accepted && selector->isValidSelection())
		{
			QStringList genes = db.getValues("SELECT gene FROM disease_gene WHERE disease_term_id='" + selector->getId() +"'");
			ui_.gene->setText(genes.join(", "));
            updateGeneFilter();
		}
	}
}

void FilterWidget::showRoiContextMenu(QPoint pos)
{
	//set up
	QMenu menu;
	QMenu* history_menu = menu.addMenu("history");
    foreach(const QString& entry, FilterWidgetHelper::roiHistory())
	{
		history_menu->addAction(entry);
	}
	QAction* a_clear = menu.addAction(QIcon(":/Icons/Trash.png"), "clear");

	//exec
	QAction* action = menu.exec(ui_.roi->mapToGlobal(pos));
	if (action==nullptr) return;

	if (action==a_clear)
	{
		clearTargetRegion();
	}
	else if (action->parent()==history_menu)
	{
        foreach(const QString& entry, FilterWidgetHelper::roiHistory())
		{
            if (action->text()==entry)
            {
                state_.setTargetRegionInfoByName(entry, false);
                FilterWidgetHelper::setTargetRegionByName(entry, ui_.roi);
            }
		}
	}
}

void FilterWidget::setFilter(int index)
{
	if (index==0)
	{
		ui_.cascade_widget->clear();
		ui_.lab_modified->setVisible(false);
		return;
	}

	try
	{
		FilterCascade filters = FilterCascadeFile::load(filterFileName(), ui_.filters->currentText());
		ui_.cascade_widget->setFilters(filters);
	}
	catch(Exception& e)
	{
		QMessageBox::warning(this, "Invalid filter", "Filter parsing failed:\n" + e.message());
	}

	ui_.lab_modified->setHidden(true);
}

void FilterWidget::clearTargetRegion()
{
	ui_.roi->setCurrentText("none");
    state_.clearTargetRegionFilter();
}

void FilterWidget::clearFilters()
{
	reset(false);
}

void FilterWidget::clearFiltersAndRoi()
{
	reset(true);
}

void FilterWidget::loadFilters()
{
	QStringList filter_names;
	filter_names << "[none]";
	filter_names << FilterCascadeFile::names(filterFileName());

	for (int i=0; i<filter_names.count(); ++i)
	{
		QString name = filter_names[i];
		if (name=="---")
		{
			ui_.filters->insertSeparator(i);
		}
		else
		{
			ui_.filters->addItem(name, ui_.filters->count());
		}
	}
}
