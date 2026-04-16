#include "FilterWidgetCNV.h"
#include "Helper.h"
#include "NGSD.h"
#include "Log.h"
#include "PhenotypeSelectionWidget.h"
#include "GUIHelper.h"
#include "GSvarHelper.h"
#include "LoginManager.h"
#include <QCompleter>
#include <QMenu>
#include <QDialog>
#include <QMessageBox>
#include <QSortFilterProxyModel>
#include "FilterWidgetHelper.h"
#include "PhenotypeSettingsDialog.h"


FilterWidgetCNV::FilterWidgetCNV(QWidget *parent)
	: QWidget(parent)
	, ui_()
	, data_controller_(AnalysisDataController::instance())
	, state_(AnalysisDataController::instance().getSmallVariantsFilterState())
{
	ui_.setupUi(this);

	//connect gui changes to filter state:
	ui_.cascade_widget->setSubject(VariantType::CNVS);
	connect(ui_.cascade_widget, SIGNAL(filterCascadeChanged()), this, SLOT(updateFilterName()));
	connect(ui_.cascade_widget, SIGNAL(filterCascadeChanged()), this, SLOT(updateFilterCascade()));
	connect(ui_.cascade_widget, SIGNAL(customFilterLoaded()), this, SLOT(customFilterLoaded()));
	connect(ui_.filters, SIGNAL(currentIndexChanged(int)), this, SLOT(setFilter(int)));
	connect(&state_, SIGNAL(targetRegionChanged(const TargetRegionInfo&)), this, SLOT(updateGeneWarning()));
	connect(ui_.gene, SIGNAL(editingFinished()), this, SLOT(updateGeneFilter()));
	connect(ui_.text, SIGNAL(editingFinished()), this, SLOT(updateTextFilter()));
	connect(ui_.region, SIGNAL(editingFinished()), this, SLOT(updateRegionFilter()));
	connect(ui_.report_config, SIGNAL(currentIndexChanged(int)), this, SLOT(updateReportConfigfilter()));

	ui_.lab_modified->setHidden(true);

	connect(ui_.hpo_import, SIGNAL(clicked(bool)), this, SLOT(importHPO()));
	connect(ui_.roi_import, SIGNAL(clicked(bool)), this, SLOT(importROI()));
	connect(ui_.region_import, SIGNAL(clicked(bool)), this, SLOT(importRegion()));
	connect(ui_.gene_import, SIGNAL(clicked(bool)), this, SLOT(importGene()));
	connect(ui_.text_import, SIGNAL(clicked(bool)), this, SLOT(importText()));
	connect(ui_.report_config, SIGNAL(currentIndexChanged(int)), this, SIGNAL(filtersChanged()));
	connect(ui_.calculate_gene_overlap, SIGNAL(clicked(bool)), this, SIGNAL(calculateGeneTargetRegionOverlap()));

	connect(ui_.hpo, SIGNAL(clicked(QPoint)), this, SLOT(editPhenotypes()));
	ui_.hpo->setEnabled(LoginManager::active());

	// connect changes in state to the gui
	connect(&state_, SIGNAL(filterNameChanged(const QString&)), this, SLOT(updateFilterName()));
	connect(&state_, SIGNAL(filterCascadeChanged(const FilterCascade&)), this, SLOT(updateFilterCascade()));
	connect(&state_, SIGNAL(targetRegionChanged(const TargetRegionInfo&)), this, SLOT(updateTargetRegionFilter(const TargetRegionInfo&)));
	connect(&state_, SIGNAL(genesChanged(const GeneSet&)), this, SLOT(updateFilterName()));
	connect(&state_, SIGNAL(regionFilterChanged(const BedLine&)), this, SLOT(updateRegionFilter()));
	connect(&state_, SIGNAL(phenotypesChanged(const PhenotypeList&)), this, SLOT(phenotypesChanged()));
	connect(&state_, SIGNAL(reportConfigFilterChanged(const ReportConfigFilter&)), this, SLOT(updateReportConfigfilter()));

    FilterWidgetHelper::loadTargetRegions(ui_.roi);
	loadFilters();
	reset(true);
}

void FilterWidgetCNV::resetSignalsUnblocked(bool clear_roi)
{
	//filters
	ui_.filters->setCurrentIndex(0);
	ui_.cascade_widget->clear();

    //rois
	if (clear_roi)
	{
		ui_.roi->setCurrentIndex(1);
	}

    //gene
    ui_.gene->clear();
	ui_.text->clear();
	ui_.region->clear();

}

QString FilterWidgetCNV::filterFileName() const
{
    return GSvarHelper::applicationBaseName() + "_filters_cnv.ini";
}

void FilterWidgetCNV::reset(bool clear_roi)
{
	blockSignals(true);
	resetSignalsUnblocked(clear_roi);
	blockSignals(false);
}

void FilterWidgetCNV::markFailedFilters()
{
	ui_.cascade_widget->markFailedFilters();
}

void FilterWidgetCNV::setTargetRegionByDisplayName(QString name)
{
	QString system = "Processing system: " + name;
	QString subpanel ="Sub-panel: " + name;

	for (int i=0; i<ui_.roi->count(); ++i)
	{
		if (ui_.roi->itemText(i)==system || ui_.roi->itemText(i)==subpanel)
		{
			ui_.roi->setCurrentIndex(i);
			break;
		}
	}
}

bool FilterWidgetCNV::setTargetRegionByName(QString name)
{
	return FilterWidgetHelper::setTargetRegionByName(name, ui_.roi);
}

void FilterWidgetCNV::updateFilterName()
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

void FilterWidgetCNV::updateFilterCascade()
{
	state_.setFilterCascade(ui_.cascade_widget->filters(), false);
}

void FilterWidgetCNV::updateTargetRegionFilter(int index)
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

void FilterWidgetCNV::updateTargetRegionFilter(const TargetRegionInfo& new_target)
{
	FilterWidgetHelper::setTargetRegionByName(new_target.name, ui_.roi);
}

void FilterWidgetCNV::updateRegionFilter()
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

void FilterWidgetCNV::updateGeneFilter()
{
	state_.setGenes(GeneSet::createFromText(ui_.gene->displayText().toUtf8(), ','), false);
}

void FilterWidgetCNV::updateTextFilter()
{
	state_.setTextFilter(ui_.text->displayText(), false);
}

void FilterWidgetCNV::updateReportConfigfilter()
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

void FilterWidgetCNV::phenotypesChanged()
{
	//update phenotype history
	FilterWidgetHelper::updatePhenotypeHistory(state_.getPhenotypes());

	//update GUI
	QByteArrayList tmp;
	for (const Phenotype& pheno : std::as_const(state_.getPhenotypes()))
	{
		tmp << pheno.name();
	}

	ui_.hpo->setText(tmp.join("; "));

	QString tooltip = "Phenotype/inheritance filter based on HPO terms.<br><br>Notes:<br>- This functionality is only available when NGSD is enabled.<br>- Filters based on the phenotype-associated gene loci including 5000 flanking bases.";
	if (!state_.getPhenotypes().isEmpty())
	{
		tooltip += "<br><br><nobr>Selected HPO terms:</nobr>";
		for (const Phenotype& pheno : state_.getPhenotypes())
		{
			tooltip += "<br><nobr>" + pheno.toString() + "</nobr>";
		}

		tooltip += "<br><br><nobr>Selected phenotype-gene sources:</nobr>";
		tooltip += "<br><nobr>";
		for (const PhenotypeSource& s : state_.getPhenotypeSettings().sources)
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
	ui_.hpo->setToolTip(tooltip);

	//show icon if settings are changed
	static QAction* settings_action = new QAction(QIcon(":/Icons/settings.png"), "");
	if (state_.getPhenotypeSettings() != PhenotypeSettings())
	{
		ui_.hpo->addAction(settings_action, QLineEdit::TrailingPosition);
	}
	else
	{
		ui_.hpo->removeAction(settings_action);
	}
}

void FilterWidgetCNV::editPhenotypes()
{
	//edit
	PhenotypeSelectionWidget* selector = new PhenotypeSelectionWidget(this);
	selector->setPhenotypes(state_.getPhenotypes());
	auto dlg = GUIHelper::createDialog(selector, "Select HPO terms", "", true);

	//update
	if (dlg->exec()==QDialog::Accepted)
	{
		state_.setPhenotypes(selector->selectedPhenotypes());
	}
}

void FilterWidgetCNV::showPhenotypeContextMenu(QPoint pos)
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
	QAction* action = menu.exec(ui_.hpo->mapToGlobal(pos));
	if (action==nullptr) return;

	if (action==a_clear)
	{
		state_.clearPhenotypeFilter();
	}
	else if (action==a_load)
	{
		state_.setPhenotypes(data_controller_.getSamplePhenotypes());
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
			if (action->text()==entry.toString()) state_.setPhenotypes(entry);
		}
	}
}

void FilterWidgetCNV::showRoiContextMenu(QPoint pos)
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
			if (action->text()==entry) setTargetRegionByDisplayName(entry);
		}
	}
}

void FilterWidgetCNV::importHPO()
{
	state_.setPhenotypes(data_controller_.getSmallVariantsFilterState().getPhenotypes());
}

void FilterWidgetCNV::importROI()
{
	ui_.roi->setCurrentText(data_controller_.getSmallVariantsFilterState().getTargetRegionInfo().name);
}

void FilterWidgetCNV::importRegion()
{
	ui_.region->setText(data_controller_.getSmallVariantsFilterState().getRegionFilter().toString(true));
}

void FilterWidgetCNV::importGene()
{
	ui_.gene->setText(data_controller_.getSmallVariantsFilterState().getGenes().join(", "));
}

void FilterWidgetCNV::importText()
{
	ui_.text->setText(data_controller_.getSmallVariantsFilterState().getTextFilter());
}

void FilterWidgetCNV::customFilterLoaded()
{
	ui_.filters->blockSignals(true);
	ui_.filters->setCurrentIndex(0);
	ui_.filters->blockSignals(false);

	ui_.lab_modified->setHidden(false);
}

void FilterWidgetCNV::setFilter(int index)
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

void FilterWidgetCNV::clearTargetRegion()
{
	ui_.roi->setCurrentText("none");
}

void FilterWidgetCNV::loadFilters()
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
