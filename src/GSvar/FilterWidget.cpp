#include "FilterWidget.h"
#include "Settings.h"
#include "Helper.h"
#include "NGSD.h"
#include "Log.h"
#include "ScrollableTextDialog.h"
#include "PhenotypeSelectionWidget.h"
#include "GUIHelper.h"
#include "GSvarHelper.h"
#include <QFileInfo>
#include <QFileDialog>
#include <QCompleter>
#include <QMenu>
#include <QMessageBox>

FilterWidget::FilterWidget(QWidget *parent)
	: QWidget(parent)
	, ui_()
{
	ui_.setupUi(this);
	ui_.cascade_widget->setSubject(VariantType::SNVS_INDELS);

	connect(ui_.cascade_widget, SIGNAL(filterCascadeChanged()), this, SLOT(updateFilterName()));
	connect(ui_.cascade_widget, SIGNAL(filterCascadeChanged()), this, SIGNAL(filtersChanged()));
	connect(ui_.filters, SIGNAL(currentIndexChanged(int)), this, SLOT(setFilter(int)));
	ui_.lab_modified->setHidden(true);

	connect(ui_.roi_add, SIGNAL(clicked()), this, SLOT(addRoi()));
	connect(ui_.roi_add_temp, SIGNAL(clicked()), this, SLOT(addRoiTemp()));
	connect(ui_.roi_remove, SIGNAL(clicked()), this, SLOT(removeRoi()));
	connect(ui_.roi, SIGNAL(currentIndexChanged(int)), this, SLOT(roiSelectionChanged(int)));
	connect(ui_.roi_details, SIGNAL(clicked(bool)), this, SLOT(showTargetRegionDetails()));
	connect(this, SIGNAL(targetRegionChanged()), this, SLOT(updateGeneWarning()));

	connect(ui_.gene, SIGNAL(editingFinished()), this, SLOT(geneChanged()));
	connect(ui_.text, SIGNAL(editingFinished()), this, SLOT(textChanged()));
	connect(ui_.region, SIGNAL(editingFinished()), this, SLOT(regionChanged()));
	connect(ui_.report_config, SIGNAL(stateChanged(int)), this, SLOT(reportConfigFilterChanged()));

	QAction* action = new QAction("clear", this);
	connect(action, &QAction::triggered, this, &FilterWidget::clearTargetRegion);
	ui_.roi->addAction(action);

	connect(ui_.hpo_terms, SIGNAL(clicked(QPoint)), this, SLOT(editPhenotypes()));
	connect(ui_.hpo_terms, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(showPhenotypeContextMenu(QPoint)));
	ui_.hpo_terms->setEnabled(Settings::boolean("NGSD_enabled", true));

	loadTargetRegions();
	loadFilters();
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
	ui_.roi->blockSignals(true);

	//store old selection
	QString current = ui_.roi->currentText();

	ui_.roi->clear();
	ui_.roi->addItem("", "");
	ui_.roi->addItem("none", "");
	ui_.roi->insertSeparator(ui_.roi->count());

	//load ROIs of NGSD processing systems
	try
	{
		QMap<QString, QString> systems = NGSD().getProcessingSystems(true, true);
		auto it = systems.constBegin();
		while (it != systems.constEnd())
		{
			ui_.roi->addItem("Processing system: " + it.key(), Helper::canonicalPath(it.value()));
			++it;
		}
		ui_.roi->insertSeparator(ui_.roi->count());
	}
	catch (Exception& e)
	{
		Log::warn("Could not load NGSD processing system target regions: " + e.message());
	}

	//load ROIs of sub-panels
	try
	{
		QStringList subpanels = Helper::findFiles(NGSD::getTargetFilePath(true), "*.bed", false);
		subpanels.sort(Qt::CaseInsensitive);
		foreach(QString file, subpanels)
		{
			if (file.endsWith("_amplicons.bed")) continue;

			QString name = QFileInfo(file).fileName().replace(".bed", "");
			ui_.roi->addItem("Sub-panel: " + name, Helper::canonicalPath(file));
		}
		ui_.roi->insertSeparator(ui_.roi->count());
	}
	catch (Exception& e)
	{
		Log::warn("Could not load sub-panels target regions: " + e.message());
	}

	//load additional ROIs from settings
	QStringList rois = Settings::stringList("target_regions");
	std::sort(rois.begin(), rois.end(), [](const QString& a, const QString& b){return QFileInfo(a).fileName().toUpper() < QFileInfo(b).fileName().toUpper();});
	foreach(const QString& roi_file, rois)
	{
		QFileInfo info(roi_file);
		ui_.roi->addItem(info.fileName(), roi_file);
	}

	//restore old selection
	int current_index = ui_.roi->findText(current);
	if (current_index==-1) current_index = 1;
	ui_.roi->setCurrentIndex(current_index);

	ui_.roi->blockSignals(false);
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
		ui_.roi->setToolTip("");
		ui_.gene_warning->setHidden(true);
	}

    //gene
    last_genes_.clear();
    ui_.gene->clear();
	ui_.text->clear();
	ui_.region->clear();
	ui_.report_config->setCheckState(Qt::Unchecked);

	//phenotype
	phenotypes_.clear();
	phenotypesChanged();
}

QString FilterWidget::filterFileName() const
{
    return GSvarHelper::applicationBaseName() + "_filters.ini";
}

void FilterWidget::reset(bool clear_roi)
{
	blockSignals(true);
	resetSignalsUnblocked(clear_roi);
	blockSignals(false);

	emit filtersChanged();
	if (clear_roi) emit targetRegionChanged();
}

QString FilterWidget::targetRegion() const
{
	return ui_.roi->toolTip();
}

QString FilterWidget::targetRegionName() const
{
	return ui_.roi->currentText();
}

void FilterWidget::setTargetRegion(QString roi_file)
{
	roi_file = Helper::canonicalPath(roi_file);
	for (int i=0; i<ui_.roi->count(); ++i)
	{
		if (ui_.roi->itemData(i).toString()==roi_file)
		{
			ui_.roi->setCurrentIndex(i);
			break;
		}
	}

	emit targetRegionChanged();
}

GeneSet FilterWidget::genes() const
{
	return GeneSet::createFromText(ui_.gene->text().toLatin1(), ',');
}

QByteArray FilterWidget::text() const
{
	return ui_.text->text().trimmed().toLatin1();
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

bool FilterWidget::reportConfigurationVariantsOnly() const
{
	return ui_.report_config->isChecked();
}

const QList<Phenotype>& FilterWidget::phenotypes() const
{
	return phenotypes_;
}

void FilterWidget::setPhenotypes(const QList<Phenotype>& phenotypes)
{
	phenotypes_ = phenotypes;
	phenotypesChanged();
}

const FilterCascade& FilterWidget::filters() const
{
	return ui_.cascade_widget->filters();
}

void FilterWidget::addRoi()
{
	//get file to open
	QString path = Settings::path("path_regions");
	QString filename = QFileDialog::getOpenFileName(this, "Select target region file", path, "BED files (*.bed);;All files (*.*)");
	if (filename=="") return;

	//store open path
	Settings::setPath("path_regions", filename);

	//update settings
	QStringList rois = Settings::stringList("target_regions");
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
	QString path = Settings::path("path_regions");
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
	QStringList rois = Settings::stringList("target_regions");
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


	ui_.roi->setToolTip(ui_.roi->itemData(index).toString());

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
	//update GUI
	QByteArrayList tmp;
	foreach(const Phenotype& pheno, phenotypes_)
	{
		tmp << pheno.name();
	}

	ui_.hpo_terms->setText(tmp.join("; "));

	QString tooltip = "Phenotype/inheritance filter based on HPO terms.<br><br>Notes:<br>- This functionality is only available when NGSD is enabled.<br>- Filters based on the phenotype-associated gene loci including 5000 flanking bases.";
	if (!phenotypes_.isEmpty())
	{
		tooltip += "<br><br><nobr>Currently selected HPO terms:</nobr>";
		foreach(const Phenotype& pheno, phenotypes_)
		{
			tooltip += "<br><nobr>" + pheno.toString() + "</nobr>";
		}
	}
	ui_.hpo_terms->setToolTip(tooltip);

	emit filtersChanged();
}

void FilterWidget::updateFilterName()
{
	if (ui_.filters->currentText()=="[none]") return;

	ui_.lab_modified->setHidden(false);
}

void FilterWidget::showTargetRegionDetails()
{
	QString roi = targetRegion();
	if (roi=="") return;

	//create text
	QStringList text;
	text << "Target region: " + QFileInfo(roi).baseName();
	BedFile file;
	file.load(roi);
	text << "Regions: " + QString::number(file.count());
	text << "Bases: " + QString::number(file.baseCount());
	text << "";
	QString genes_file = roi.left(roi.size()-4) + "_genes.txt";
	if (QFile::exists(genes_file))
	{
		GeneSet genes = GeneSet::createFromFile(genes_file);
		text << "Genes: " + QString::number(genes.count());
		text << genes.join(", ");
	}
	else
	{
		text << "Genes: gene file not available for this target region!";
	}

	//show text
	ScrollableTextDialog dlg(this);
	dlg.setWindowTitle("Target region details");
	dlg.setText(text.join("\n"));
	dlg.exec();
}

void FilterWidget::updateGeneWarning()
{
	QStringList warnings;

	QString roi = targetRegion();
	if (roi!="")
	{
		QString genes_file = roi.left(roi.size()-4) + "_genes.txt";
		if (QFile::exists(genes_file))
		{
			GeneSet roi_genes = GeneSet::createFromFile(genes_file);

			//check non-coding in GRCh37
			{
				GeneSet non_coding;
				non_coding << "PADI6" << "SRD5A2" << "SYN2" << "NEFL" << "ABO" << "NR2E3" << "TTC25";
				GeneSet inter = roi_genes.intersect(non_coding);
				if (!inter.isEmpty())
				{
					warnings.append("Some genes (" + inter.join(", ") + ") of the target region are non-coding in the Ensembl annotation of GRCh37, but coding for GRCh38.\nVariants in non-coding genes have LOW/MODIFIER impact. Make sure to check these variants too!");
				}
			}

			//check "indikationsspezifische Abrechnung"
			{
				GeneSet billing;
				billing << "ACTA2" << "COL3A1" << "FBN1" << "MYH11" << "MYLK" << "SMAD3" << "TGFB2" << "TGFBR1" << "TGFBR2" << "MLH1" << "MSH2" << "MSH6" << "PMS2" << "GJB2" << "GJB6" << "SMN1" << "SMN2" << "F8" << "CNBP" << "DMPK" << "HTT" << "PTPN11" << "FMR1" << "SOS1" << "RAF1" << "RIT1" << "BRAF" << "KRAS" << "CFTR" << "DMD";
				GeneSet inter = roi_genes.intersect(billing);
				if (!inter.isEmpty())
				{
					warnings.append("Some genes (" + inter.join(", ") + ") of the target region require 'indikationsspezifische Abrechnung'!");
				}
			}
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
	if (Settings::boolean("NGSD_enabled", true))
	{
		menu.addAction("load from NGSD");
		menu.addAction("create sub-panel");
		menu.addSeparator();
	}
	if (!phenotypes_.isEmpty())
	{
		menu.addAction("clear");
	}

	//exec
	QAction* action = menu.exec(ui_.hpo_terms->mapToGlobal(pos));
	if (action==nullptr) return;

	if (action->text()=="clear")
	{
		phenotypes_.clear();
		phenotypesChanged();
	}
	else if (action->text()=="load from NGSD")
	{
		emit phenotypeImportNGSDRequested();
	}
	else if (action->text()=="create sub-panel")
	{
		emit phenotypeSubPanelRequested();
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
