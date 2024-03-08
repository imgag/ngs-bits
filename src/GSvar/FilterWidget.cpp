#include "FilterWidget.h"
#include "Settings.h"
#include "Helper.h"
#include "NGSD.h"
#include "Log.h"
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
#include "GlobalServiceProvider.h"
#include "IgvSessionManager.h"
#include <QClipboard>

FilterWidget::FilterWidget(QWidget *parent)
	: QWidget(parent)
	, ui_()
	, roi_()
{
	ui_.setupUi(this);
	ui_.cascade_widget->setSubject(VariantType::SNVS_INDELS);

	connect(ui_.cascade_widget, SIGNAL(filterCascadeChanged()), this, SLOT(updateFilterName()));
	connect(ui_.cascade_widget, SIGNAL(filterCascadeChanged()), this, SIGNAL(filtersChanged()));
	connect(ui_.cascade_widget, SIGNAL(customFilterLoaded()), this, SLOT(customFilterLoaded()));
	connect(ui_.filters, SIGNAL(currentIndexChanged(int)), this, SLOT(setFilter(int)));
	ui_.lab_modified->setHidden(true);

	connect(ui_.roi_add, SIGNAL(clicked()), this, SLOT(addRoi()));
	connect(ui_.roi_add_temp, SIGNAL(clicked()), this, SLOT(addRoiTemp()));
	connect(ui_.roi_remove, SIGNAL(clicked()), this, SLOT(removeRoi()));
	connect(ui_.roi, SIGNAL(currentIndexChanged(int)), this, SLOT(roiSelectionChanged(int)));
	connect(this, SIGNAL(targetRegionChanged()), this, SLOT(updateGeneWarning()));

	connect(ui_.gene, SIGNAL(editingFinished()), this, SLOT(geneChanged()));
	connect(ui_.text, SIGNAL(editingFinished()), this, SLOT(textChanged()));
	connect(ui_.region, SIGNAL(editingFinished()), this, SLOT(regionChanged()));
	connect(ui_.report_config, SIGNAL(currentIndexChanged(int)), this, SLOT(reportConfigFilterChanged()));

	QAction* action = new QAction(QIcon(":/Icons/Trash.png"), "clear");
	connect(action, &QAction::triggered, this, &FilterWidget::clearTargetRegion);
	ui_.roi->addAction(action);

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
	loadTargetRegions(ui_.roi);
}

void FilterWidget::loadTargetRegions(QComboBox* box)
{
	box->blockSignals(true);

	//store old selection
	QString current = box->currentText();

	box->clear();
	box->addItem("", "");
	box->addItem("none", "");
	box->insertSeparator(box->count());

	if (GlobalServiceProvider::database().enabled())
	{
		NGSD db;

		//load ROIs of NGSD processing systems
		SqlQuery query = db.getQuery();
		query.exec("SELECT name_manufacturer, target_file FROM processing_system ORDER by name_manufacturer ASC");
		while(query.next())
		{
			QString name = query.value(0).toString();
			QString roi = query.value(1).toString().trimmed();
			if (roi.isEmpty()) continue;

			box->addItem("Processing system: " + name, "Processing system: " + name);
		}
		box->insertSeparator(box->count());

		//load ROIs of NGSD sub-panels
		foreach(const QString& subpanel, db.subPanelList(false))
		{
			box->addItem("Sub-panel: " + subpanel, "Sub-panel: " + subpanel);
		}
		box->insertSeparator(box->count());
	}

	//load additional ROIs from settings
	QStringList rois = Settings::stringList("target_regions", true);
	std::sort(rois.begin(), rois.end(), [](const QString& a, const QString& b){return QFileInfo(a).fileName().toUpper() < QFileInfo(b).fileName().toUpper();});
	foreach(const QString& roi_file, rois)
	{
		QFileInfo info(roi_file);
		box->addItem(info.fileName(), roi_file);
	}

	//restore old selection
	int current_index = box->findText(current);
	if (current_index==-1) current_index = 1;
	box->setCurrentIndex(current_index);

	box->blockSignals(false);
}

void FilterWidget::loadTargetRegionData(TargetRegionInfo& roi, QString name)
{
	roi.clear();

	if (name.trimmed()=="") return;

	if (name.startsWith("Sub-panel: "))
	{
		roi.name = name.split(":")[1].trimmed();

		NGSD db;
		roi.regions = db.subpanelRegions(roi.name);
		roi.regions.merge();

		roi.genes = db.subpanelGenes(roi.name);
	}
	else if (name.startsWith("Processing system: "))
	{
		roi.name = name.split(":")[1].trimmed();

		NGSD db;
		int sys_id = db.processingSystemId(roi.name);
		roi.regions = GlobalServiceProvider::database().processingSystemRegions(sys_id, false);
		roi.regions.merge();
		roi.genes = GlobalServiceProvider::database().processingSystemGenes(sys_id, true);
	}
	else //local target regions
	{
		roi.name = QFileInfo(name).baseName();

		roi.regions.load(name);
		roi.regions.merge();

		QString genes_file = name.left(name.size()-4) + "_genes.txt";
		if (QFile::exists(genes_file))
		{
			roi.genes = GeneSet::createFromFile(genes_file);
		}
	}
}

void FilterWidget::checkGeneNames(const GeneSet& genes, QLineEdit* widget)
{
	if (!GlobalServiceProvider::database().enabled()) return;

	QStringList errors;
	NGSD db;
	foreach(const QByteArray& gene, genes)
	{
		if (!db.approvedGeneNames().contains(gene))
		{
			QByteArray approved = db.geneToApproved(gene, false);
			if (approved!="")
			{
				errors << "Gene symbol " + gene + " is not an approved HGNC symbol! Please use " + approved + "!";
			}
		}
	}
	if (errors.isEmpty())
	{
		widget->setToolTip("");
		widget->setStyleSheet("");
	}
	else
	{
		widget->setToolTip(errors.join("\n"));
		widget->setStyleSheet("QLineEdit {border: 2px solid red;}");
	}
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
		roi_.clear();
		ui_.gene_warning->setHidden(true);
	}

	//gene
	last_genes_.clear();
	ui_.gene->clear();
	ui_.text->clear();
	ui_.region->clear();
	ui_.report_config->setCurrentIndex(0);

	//phenotype
	phenotypes_.clear();
	phenotypesChanged();
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

	emit filtersChanged();
	if (clear_roi) emit targetRegionChanged();
}

const TargetRegionInfo& FilterWidget::targetRegion() const
{
	return roi_;
}

QString FilterWidget::targetRegionDisplayName() const
{
	return ui_.roi->currentText();
}

bool FilterWidget::setTargetRegionByDisplayName(QString name)
{
	if (name.endsWith(".bed")) name = name.left(name.size()-4);

	QString system = "Processing system: " + name;
	QString subpanel ="Sub-panel: " + name;

	for (int i=0; i<ui_.roi->count(); ++i)
	{
		if (ui_.roi->itemText(i)==system || ui_.roi->itemText(i)==subpanel)
		{
			ui_.roi->setCurrentIndex(i);
			return true;
		}
	}

	return false;
}

GeneSet FilterWidget::genes() const
{
	return GeneSet::createFromText(ui_.gene->text().toUtf8(), ',');
}

QByteArray FilterWidget::text() const
{
	return ui_.text->text().trimmed().toUtf8();
}

QString FilterWidget::region() const
{
	return ui_.region->text().trimmed();
}

void FilterWidget::setRegion(QString region)
{
	ui_.region->setText(region);
	regionChanged();
}

ReportConfigFilter FilterWidget::reportConfigurationFilter() const
{
	if (ui_.report_config->currentIndex()==1)
	{
		return ReportConfigFilter::HAS_RC;
	}
	else if (ui_.report_config->currentIndex()==2)
	{
		return ReportConfigFilter::NO_RC;
	}

	return ReportConfigFilter::NONE;
}

void FilterWidget::disableReportConfigurationFilter() const
{
	ui_.report_config->setCurrentIndex(0);
	ui_.report_config->setEnabled(false);
}

const PhenotypeList& FilterWidget::phenotypes() const
{
	return phenotypes_;
}

void FilterWidget::setPhenotypes(const PhenotypeList& phenotypes)
{
	phenotypes_ = phenotypes;
	phenotypesChanged();
}

const PhenotypeSettings&FilterWidget::phenotypeSettings() const
{
	return phenotype_settings_;
}

void FilterWidget::setPhenotypeSettings(const PhenotypeSettings& settings)
{
	phenotype_settings_ = settings;
	phenotypesChanged();
}

const FilterCascade& FilterWidget::filters() const
{
	return ui_.cascade_widget->filters();
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
	emit filtersChanged();
}

void FilterWidget::roiSelectionChanged(int index)
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

		QCompleter* completer = new QCompleter(ui_.roi->model(), ui_.roi);
		completer->setCompletionMode(QCompleter::PopupCompletion);
		completer->setCaseSensitivity(Qt::CaseInsensitive);
		completer->setFilterMode(Qt::MatchContains);
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
		loadTargetRegionData(roi_, roi_name);
	}
	catch(Exception& e)
	{
		QMessageBox::warning(this, "Error loading target region '" + roi_.name + "'", e.message());
		clearTargetRegion();
	}

	if(index!=0)
	{
		emit filtersChanged();
		emit targetRegionChanged();
	}
}

void FilterWidget::geneChanged()
{
	if (genes()!=last_genes_)
	{
		last_genes_ = genes();
		checkGeneNames(last_genes_, ui_.gene);
		emit filtersChanged();
	}
}

void FilterWidget::textChanged()
{
	emit filtersChanged();
}

void FilterWidget::regionChanged()
{
	emit filtersChanged();
}

void FilterWidget::reportConfigFilterChanged()
{
	emit filtersChanged();
}

void FilterWidget::phenotypesChanged()
{
	//update phenotype list
	QByteArrayList tmp;
	foreach(const Phenotype& pheno, phenotypes_)
	{
		tmp << pheno.name();
	}
	ui_.hpo_terms->setText(tmp.join("; "));

	//update tooltip
	QString tooltip = "Phenotype filter based on HPO terms.<br><br>Notes:<br>- This functionality is only available when NGSD is enabled.<br>- Filters based on phenotype-associated gene loci including 5000 flanking bases.";
	if (!phenotypes_.isEmpty())
	{
		tooltip += "<br><br><nobr>Selected HPO terms:</nobr>";
		foreach(const Phenotype& pheno, phenotypes_)
		{
			tooltip += "<br><nobr>" + pheno.toString() + "</nobr>";
		}

		tooltip += "<br><br><nobr>Selected phenotype-gene sources:</nobr>";
		tooltip += "<br><nobr>";
		foreach(const PhenotypeSource& s, phenotype_settings_.sources)
		{
			tooltip += Phenotype::sourceToString(s) + ", ";
		}
		tooltip.chop(2);
		tooltip += "</nobr>";

		tooltip += "<br><br><nobr>Selected phenotype-gene evidence levels:</nobr>";
		tooltip += "<br><nobr>";
		foreach(const PhenotypeEvidenceLevel& e, phenotype_settings_.evidence_levels)
		{
			tooltip += Phenotype::evidenceToString(e) + ", ";
		}
		tooltip.chop(2);
		tooltip += "</nobr>";

		tooltip += "<br><br><nobr>Selected phenotype combination mode:</nobr>";
		tooltip += QString("<br>") + (phenotype_settings_.mode==PhenotypeCombimnationMode::MERGE ? "merge" : "intersect");
	}
	ui_.hpo_terms->setToolTip(tooltip);

	//show icon if settings are changed
	static QAction* settings_action = new QAction(QIcon(":/Icons/settings.png"), "");
	if (phenotype_settings_!=PhenotypeSettings())
	{
		ui_.hpo_terms->addAction(settings_action, QLineEdit::TrailingPosition);
	}
	else
	{
		ui_.hpo_terms->removeAction(settings_action);
	}

	emit filtersChanged();
}

void FilterWidget::updateFilterName()
{
	if (ui_.filters->currentText()=="[none]") return;

	ui_.lab_modified->setHidden(false);
}

void FilterWidget::customFilterLoaded()
{
	ui_.filters->blockSignals(true);
	ui_.filters->setCurrentIndex(0);
	ui_.filters->blockSignals(false);

	ui_.lab_modified->setHidden(false);

	emit filtersChanged();
}

void FilterWidget::showTargetRegionDetails()
{
	if (!roi_.isValid()) return;

	//ROI statistics
	QStringList text;
	text << "Target region: " + roi_.name;
	text << "Regions: " + QString::number(roi_.regions.count());
	text << "Bases: " + QString::number(roi_.regions.baseCount());
	text << "";

	if (roi_.genes.count()!=0)
	{
		text << "Genes: " + QString::number(roi_.genes.count());
		text << roi_.genes.join(", ");
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
	QApplication::clipboard()->setText(roi_.genes.toStringList().join("\n"));
}

void FilterWidget::copyTargetRegionToClipboard()
{
	QApplication::clipboard()->setText(roi_.regions.toText());
}

void FilterWidget::openTargetRegionInIGV()
{

	QString roi_file = GSvarHelper::localRoiFolder() + roi_.name + ".bed";
	roi_.regions.store(roi_file);

    IgvSessionManager::get(0).loadFileInIGV(roi_file, false);
}

void FilterWidget::updateGeneWarning()
{
	QStringList warnings;

	//indikationsspezifische Abrechnung
	if (roi_.isValid() && !roi_.genes.isEmpty())
	{
		QString warning = GSvarHelper::specialGenes(roi_.genes);
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
	selector->setPhenotypes(phenotypes_);

	auto dlg = GUIHelper::createDialog(selector, "Select HPO terms", "", true);

	//update
	if (dlg->exec()==QDialog::Accepted)
	{
		phenotypes_ = selector->selectedPhenotypes();
		phenotypesChanged();
	}
}

void FilterWidget::showPhenotypeContextMenu(QPoint pos)
{
	//set up
	QMenu menu;
	QAction* a_load = LoginManager::active() ? menu.addAction(QIcon(":/Icons/NGSD.png"), "load from NGSD") : nullptr;
	QAction* a_subpanel = LoginManager::active() ? menu.addAction("create sub-panel") : nullptr;
	menu.addSeparator();
	QAction* a_settings = menu.addAction(QIcon(":/Icons/settings.png"), "settings");
	QAction* a_clear = phenotypes_.isEmpty() ? nullptr : menu.addAction(QIcon(":/Icons/Trash.png"), "clear");

	//exec
	QAction* action = menu.exec(ui_.hpo_terms->mapToGlobal(pos));
	if (action==nullptr) return;

	if (action==a_clear)
	{
		phenotypes_.clear();
		phenotype_settings_.revert();
		phenotypesChanged();
	}
	else if (action==a_load)
	{
		emit phenotypeImportNGSDRequested();
	}
	else if (action==a_subpanel)
	{
		emit phenotypeSubPanelRequested();
	}
	else if (action==a_settings)
	{
		PhenotypeSettingsDialog dlg(this);
		dlg.set(phenotype_settings_);

		//update
		if (dlg.exec()==QDialog::Accepted)
		{
			phenotype_settings_ = dlg.get();
			phenotypesChanged();
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
		menu.addAction("clear");
	}

	//exec
	QAction* action = menu.exec(ui_.gene->mapToGlobal(pos));
	if (action==nullptr) return;

	if (action->text()=="clear")
	{
		ui_.gene->setText("");
		geneChanged();
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
			geneChanged();
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
