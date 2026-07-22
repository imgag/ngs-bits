#include "FilterWidgetSV.h"
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

FilterWidgetSV::FilterWidgetSV(QWidget *parent)
	: QWidget(parent)
	, ui_()
	, data_controller_(AnalysisDataController::instance())
	, state_(data_controller_.getSvFilterState())
{
	ui_.setupUi(this);
	ui_.cascade_widget->setSubject(VariantType::SVS);
	//connect gui changes to filter state
	connect(ui_.cascade_widget, SIGNAL(filterCascadeChanged()), this, SLOT(updateStateFilterName()));
	connect(ui_.cascade_widget, SIGNAL(filterCascadeChanged()), this, SLOT(updateStateFilterCascade()));
	connect(ui_.gene, SIGNAL(editingFinished()), this, SLOT(updateStateGeneFilter()));
	connect(ui_.text, SIGNAL(editingFinished()), this, SLOT(updateStateTextFilter()));
	connect(ui_.region, SIGNAL(editingFinished()), this, SLOT(updateStateRegionFilter()));
	connect(ui_.report_config, SIGNAL(currentIndexChanged(int)), this, SLOT(updateStateReportConfigfilter()));
	connect(ui_.hpo, SIGNAL(clicked(QPoint)), this, SLOT(editPhenotypes()));
	connect(ui_.roi, SIGNAL(currentIndexChanged(int)), this, SLOT(updateStateTargetRegionFilter(int)));

	// connect changes in state to the gui
	connect(&state_, SIGNAL(filterNameChanged(const QString&)), this, SLOT(updateGuiFilterName()));
	connect(&state_, SIGNAL(filterCascadeChanged(const FilterCascade&)), this, SLOT(updateGuiFilterCascade()));
	connect(&state_, SIGNAL(targetRegionChanged(const TargetRegionInfo&)), this, SLOT(updateGuiTargetRegionFilter()));
	connect(&state_, SIGNAL(genesChanged(const GeneSet&)), this, SLOT(updateGuiGeneFilter()));
	connect(&state_, SIGNAL(regionFilterChanged(const BedLine&)), this, SLOT(updateGuiRegionFilter()));
	connect(&state_, SIGNAL(phenotypesChanged(PhenotypeList)), this, SLOT(updateGuiPhenotypes()));
	connect(&state_, SIGNAL(reportConfigFilterChanged(const ReportConfigFilter&)), this, SLOT(updateGuiReportConfigfilter()));

	ui_.lab_modified->setHidden(true);

	connect(ui_.hpo_import, SIGNAL(clicked(bool)), this, SLOT(importHPO()));
	connect(ui_.roi_import, SIGNAL(clicked(bool)), this, SLOT(importROI()));
	connect(ui_.region_import, SIGNAL(clicked(bool)), this, SLOT(importRegion()));
	connect(ui_.gene_import, SIGNAL(clicked(bool)), this, SLOT(importGene()));
	connect(ui_.text_import, SIGNAL(clicked(bool)), this, SLOT(importText()));
	connect(ui_.roi, SIGNAL(currentIndexChanged(int)), this, SLOT(updateTargetRegionFilter(int)));
	connect(ui_.calculate_gene_overlap, SIGNAL(clicked(bool)), this, SIGNAL(calculateGeneTargetRegionOverlap()));
	connect(ui_.hpo, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(showPhenotypeContextMenu(QPoint)));
	connect(ui_.roi, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(showRoiContextMenu(QPoint)));

	ui_.hpo->setEnabled(LoginManager::active());

	//set available filter column values
	ui_.cascade_widget->setValidFilterEntries(data_controller_.getValidSvFilterEntries());

	FilterWidgetHelper::loadTargetRegions(ui_.roi);
	loadFilters();
	reset(true);
}

void FilterWidgetSV::resetSignalsUnblocked(bool clear_roi)
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

QString FilterWidgetSV::filterFileName() const
{
	return GSvarHelper::applicationBaseName() + "_filters_sv.ini";
}

void FilterWidgetSV::reset(bool clear_roi)
{
	blockSignals(true);
	resetSignalsUnblocked(clear_roi);
	blockSignals(false);
}

void FilterWidgetSV::markFailedFilters()
{
	ui_.cascade_widget->markFailedFilters();
}

void FilterWidgetSV::setTargetRegionByDisplayName(QString name)
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

bool FilterWidgetSV::setTargetRegionByName(QString name)
{
	return FilterWidgetHelper::setTargetRegionByName(name, ui_.roi);
}

void FilterWidgetSV::updateStateFilterName()
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

void FilterWidgetSV::updateGuiFilterName()
{
	if (state_.getFilterName() == "")
	{
		ui_.filters->setCurrentText("[none]");
		ui_.lab_modified->setHidden(true);
	}
	else
	{
		int idx = ui_.filters->findText(state_.getFilterName(), Qt::MatchExactly);
		if (idx == -1)
		{
			THROW(ProgrammingException, "SV filter state name was set to '" + state_.getFilterName() + "' this is not an option in the ui_.filters Combobox.");
		}

		ui_.filters->setCurrentIndex(idx);

		FilterCascade base_filters = FilterCascadeFile::load(filterFileName(), ui_.filters->currentText());
		if (state_.getFilterCascade() != base_filters)
		{
			ui_.lab_modified->setHidden(false);
		}
	}
}

void FilterWidgetSV::updateStateFilterCascade()
{
	state_.setFilterCascade(ui_.cascade_widget->filters(), false);
}

void FilterWidgetSV::updateGuiFilterCascade()
{
	ui_.cascade_widget->setFilters(state_.getFilterCascade());
}

void FilterWidgetSV::updateStateTargetRegionFilter(int index)
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
		QMessageBox::warning(this, "Error loading target region '" + roi_name + "'", e.message());
		clearTargetRegion();
	}

	//enable annotation button if annotation is possible
	ui_.calculate_gene_overlap->setEnabled(LoginManager::active() && !state_.getTargetRegionInfo().genes.isEmpty());
}

void FilterWidgetSV::updateGuiTargetRegionFilter()
{
	FilterWidgetHelper::setTargetRegionByName(state_.getTargetRegionInfo().name, ui_.roi);
}

void FilterWidgetSV::updateStateRegionFilter()
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

void FilterWidgetSV::updateGuiRegionFilter()
{
	ui_.region->setText(state_.getRegionFilter().toString(true));
}

void FilterWidgetSV::updateStateGeneFilter()
{
	state_.setGenes(GeneSet::createFromText(ui_.gene->displayText().toUtf8(), ','), false);
}

void FilterWidgetSV::updateGuiGeneFilter()
{
	ui_.gene->setText(state_.getGenes().toString(", "));
}

void FilterWidgetSV::updateStateTextFilter()
{
	state_.setTextFilter(ui_.text->displayText(), false);
}

void FilterWidgetSV::updateGuiTextFilter()
{
	ui_.text->setText(state_.getTextFilter());
}

void FilterWidgetSV::updateStateReportConfigfilter()
{
	if (ui_.report_config->currentIndex() == 0)
	{
		state_.setReportConfigFilter(ReportConfigFilter::NONE, false);
	}
	else if (ui_.report_config->currentIndex() == 1)
	{
		state_.setReportConfigFilter(ReportConfigFilter::HAS_RC, false);
	}
	else  if (ui_.report_config->currentIndex() == 2)
	{
		state_.setReportConfigFilter(ReportConfigFilter::NO_RC, false);
	}
	else
	{
		THROW(ArgumentException, "Value in report config QComboBox coundn't be translated to a ReportConfigFilter type");
	}
}

void FilterWidgetSV::updateGuiReportConfigfilter()
{
	ReportConfigFilter current_state = state_.getReportConfigFilter();

	if (current_state == ReportConfigFilter::NONE)
	{
		ui_.report_config->setCurrentIndex(0);
	}
	else if (current_state == ReportConfigFilter::HAS_RC)
	{
		ui_.report_config->setCurrentIndex(1);
	}
	else  if (current_state == ReportConfigFilter::NO_RC)
	{
		ui_.report_config->setCurrentIndex(2);
	}
	else
	{
		THROW(ArgumentException, "Value in report config QComboBox coundn't be translated to a ReportConfigFilter type");
	}
}

void FilterWidgetSV::updateGuiPhenotypes()
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

void FilterWidgetSV::editPhenotypes()
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

void FilterWidgetSV::showPhenotypeContextMenu(QPoint pos)
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

	//exec
	QAction* action = menu.exec(ui_.hpo->mapToGlobal(pos));
	if (action==nullptr) return;

	if (action==a_load)
	{
		state_.setPhenotypes(data_controller_.getSamplePhenotypes());
	}
	else if (action==a_clear)
	{
		state_.clearPhenotypeFilter();
	}
	else if (action->parent()==history_menu)
	{
		foreach(const PhenotypeList& entry, FilterWidgetHelper::phenotypeHistory())
		{
			if (action->text()==entry.toString()) state_.setPhenotypes(entry);
		}
	}
}

void FilterWidgetSV::showRoiContextMenu(QPoint pos)
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

void FilterWidgetSV::importHPO()
{
	state_.setPhenotypes(data_controller_.getSmallVariantsFilterState().getPhenotypes());
}

void FilterWidgetSV::importROI()
{
	ui_.roi->setCurrentText(data_controller_.getSmallVariantsFilterState().getTargetRegionInfo().name);
}

void FilterWidgetSV::importRegion()
{
	ui_.region->setText(data_controller_.getSmallVariantsFilterState().getRegionFilter().toString(true));
}

void FilterWidgetSV::importGene()
{
	ui_.gene->setText(data_controller_.getSmallVariantsFilterState().getGenes().join(", "));
}

void FilterWidgetSV::importText()
{
	ui_.text->setText(data_controller_.getSmallVariantsFilterState().getTextFilter());
}

void FilterWidgetSV::customFilterLoaded()
{
	ui_.filters->blockSignals(true);
	ui_.filters->setCurrentIndex(0);
	ui_.filters->blockSignals(false);

	ui_.lab_modified->setHidden(false);
}

void FilterWidgetSV::setFilter(int index)
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

void FilterWidgetSV::clearTargetRegion()
{
	ui_.roi->setCurrentText("none");
}

void FilterWidgetSV::loadFilters()
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
