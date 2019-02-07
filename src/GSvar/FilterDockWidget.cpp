#include "FilterDockWidget.h"
#include "Settings.h"
#include "Helper.h"
#include "NGSD.h"
#include "Log.h"
#include "ScrollableTextDialog.h"
#include <QCheckBox>
#include <QFileInfo>
#include <QFileDialog>
#include <QInputDialog>
#include <QLabel>
#include <QCompleter>
#include <QMenu>
#include "PhenotypeSelectionWidget.h"
#include "GUIHelper.h"
#include "FilterEditDialog.h"

FilterDockWidget::FilterDockWidget(QWidget *parent)
	: QWidget(parent)
	, ui_()
{
	ui_.setupUi(this);

	connect(ui_.roi_add, SIGNAL(clicked()), this, SLOT(addRoi()));
	connect(ui_.roi_add_temp, SIGNAL(clicked()), this, SLOT(addRoiTemp()));
	connect(ui_.roi_remove, SIGNAL(clicked()), this, SLOT(removeRoi()));
	connect(ui_.rois, SIGNAL(currentIndexChanged(int)), this, SLOT(roiSelectionChanged(int)));
	connect(ui_.roi_details, SIGNAL(clicked(bool)), this, SLOT(showTargetRegionDetails()));
	connect(this, SIGNAL(targetRegionChanged()), this, SLOT(updateGeneWarning()));

	connect(ui_.ref_add, SIGNAL(clicked()), this, SLOT(addRef()));
	connect(ui_.ref_remove, SIGNAL(clicked()), this, SLOT(removeRef()));
	connect(ui_.refs, SIGNAL(currentIndexChanged(int)), this, SLOT(referenceSampleChanged(int)));

	connect(ui_.gene, SIGNAL(editingFinished()), this, SLOT(geneChanged()));
	connect(ui_.text, SIGNAL(editingFinished()), this, SLOT(textChanged()));
	connect(ui_.region, SIGNAL(editingFinished()), this, SLOT(regionChanged()));

	connect(ui_.filters_entries, SIGNAL(itemSelectionChanged()), this, SLOT(filterSelectionChanged()));
	connect(ui_.filters_entries, SIGNAL(itemActivated(QListWidgetItem*)), this, SLOT(editSelectedFilter()));
	connect(ui_.filters_entries, SIGNAL(itemChanged(QListWidgetItem*)), this, SLOT(toggleSelectedFilter(QListWidgetItem*)));
	connect(ui_.filter_delete, SIGNAL(pressed()), this, SLOT(deleteSelectedFilter()));
	connect(ui_.filter_edit, SIGNAL(pressed()), this, SLOT(editSelectedFilter()));
	connect(ui_.filter_add, SIGNAL(pressed()), this, SLOT(addFilter()));
	connect(ui_.filter_up, SIGNAL(pressed()), this, SLOT(moveUpSelectedFilter()));
	connect(ui_.filter_down, SIGNAL(pressed()), this, SLOT(moveDownSelectedFilter()));

	if (Settings::boolean("NGSD_enabled", true))
	{
		connect(ui_.hpo_terms, SIGNAL(clicked(QPoint)), this, SLOT(editPhenotypes()));
		connect(ui_.hpo_terms, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(showPhenotypeContextMenu(QPoint)));
	}
	else
	{
		ui_.hpo_terms->setEnabled(false);
	}

	loadTargetRegions();
	loadReferenceFiles();

	reset(true);
}

void FilterDockWidget::setValidFilterEntries(const QStringList& filter_entries)
{
	valid_filter_entries_ = filter_entries;
}

void FilterDockWidget::setFilters(const QString& name, const FilterCascade& filters)
{
	ui_.filter_name->setText(name);

	filters_ = filters;

	//overwrite valid entries of 'filter' column
	for(int i=0; i<filters_.count(); ++i)
	{
		if (filters_[i]->name()=="Filter columns")
		{
			filters_[i]->overrideConstraint("entries", "valid", valid_filter_entries_.join(','));
		}
	}

	updateGUI();
	onFilterCascadeChange(false);
}

void FilterDockWidget::markFailedFilters()
{
	for(int i=0; i<ui_.filters_entries->count(); ++i)
	{
		QListWidgetItem* item = ui_.filters_entries->item(i);

		QStringList errors = filters_.errors(i);
		if (errors.isEmpty())
		{
			item->setBackground(QBrush());
		}
		else
		{
			item->setToolTip(filters_[i]->description().join("\n") + "\n\nErrors:\n" + errors.join("\n"));
			item->setBackgroundColor(QColor(255,160,110));
		}
	}
}

void FilterDockWidget::loadTargetRegions()
{
	ui_.rois->blockSignals(true);

	//store old selection
	QString current = ui_.rois->currentText();

	ui_.rois->clear();
	ui_.rois->addItem("", "");
	ui_.rois->addItem("none", "");
	ui_.rois->insertSeparator(ui_.rois->count());

	//load ROIs of NGSD processing systems
	try
	{
		QMap<QString, QString> systems = NGSD().getProcessingSystems(true, true);
		auto it = systems.constBegin();
		while (it != systems.constEnd())
		{
			ui_.rois->addItem("Processing system: " + it.key(), Helper::canonicalPath(it.value()));
			++it;
		}
		ui_.rois->insertSeparator(ui_.rois->count());
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
			ui_.rois->addItem("Sub-panel: " + name, Helper::canonicalPath(file));
		}
		ui_.rois->insertSeparator(ui_.rois->count());
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
		ui_.rois->addItem(info.fileName(), roi_file);
	}

	//restore old selection
	int current_index = ui_.rois->findText(current);
	if (current_index==-1) current_index = 1;
	ui_.rois->setCurrentIndex(current_index);

	ui_.rois->blockSignals(false);
}

int FilterDockWidget::currentFilterIndex() const
{
	auto selection = ui_.filters_entries->selectedItems();
	if (selection.count()!=1) return -1;

	return ui_.filters_entries->row(selection[0]);
}

void FilterDockWidget::loadReferenceFiles()
{
	//store old selection
	QString current = ui_.refs->currentText();

	//load from settings
	ui_.refs->clear();
	ui_.refs->addItem("none", "");
	QStringList refs = Settings::stringList("reference_files");
	foreach(const QString& roi_file, refs)
	{
		QStringList parts = roi_file.trimmed().split("\t");
		if (parts.count()!=2) continue;
		ui_.refs->addItem(parts[0], Helper::canonicalPath(parts[1]));
	}

	//restore old selection
	int current_index = ui_.refs->findText(current);
	if (current_index==-1) current_index = 0;
    ui_.refs->setCurrentIndex(current_index);
}

void FilterDockWidget::resetSignalsUnblocked(bool clear_roi)
{
	//filter cols
	filters_.clear();
	updateGUI();

    //rois
	if (clear_roi)
	{
		ui_.rois->setCurrentIndex(1);
		ui_.rois->setToolTip("");
		ui_.gene_warning->setHidden(true);
	}

    //gene
    last_genes_.clear();
    ui_.gene->clear();
	ui_.text->clear();
	ui_.region->clear();

	//phenotype
	phenotypes_.clear();
	phenotypesChanged();

    //refs
    ui_.refs->setCurrentIndex(0);
	ui_.refs->setToolTip("");
}

void FilterDockWidget::focusFilter(int index)
{
	//no entries > return
	int filter_count = ui_.filters_entries->count();
	if (filter_count==0) return;

	if (index<0) //index too small > focus first item
	{
		ui_.filters_entries->item(0)->setSelected(true);
	}
	else if (index>=filter_count) //index too big > focus last item
	{
		ui_.filters_entries->item(filter_count-1)->setSelected(true);
	}
	else //index ok > focus selected item
	{
		ui_.filters_entries->item(index)->setSelected(true);
	}
}

void FilterDockWidget::reset(bool clear_roi)
{
	ui_.filter_name->setText("[none]");

	blockSignals(true);
	resetSignalsUnblocked(clear_roi);
	blockSignals(false);

	onFilterCascadeChange(true);
	if (clear_roi) emit targetRegionChanged();
}

QString FilterDockWidget::targetRegion() const
{
	return ui_.rois->toolTip();
}

void FilterDockWidget::setTargetRegion(QString roi_file)
{
	roi_file = Helper::canonicalPath(roi_file);
	for (int i=0; i<ui_.rois->count(); ++i)
	{
		if (ui_.rois->itemData(i).toString()==roi_file)
		{
			ui_.rois->setCurrentIndex(i);
			break;
		}
	}

	emit targetRegionChanged();
}

GeneSet FilterDockWidget::genes() const
{
	return GeneSet::createFromText(ui_.gene->text().toLatin1(), ',');
}

QByteArray FilterDockWidget::text() const
{
	return ui_.text->text().trimmed().toLatin1();
}

QString FilterDockWidget::region() const
{
	return ui_.region->text().trimmed();
}

void FilterDockWidget::setRegion(QString region)
{
	ui_.region->setText(region);
	regionChanged();
}

const QList<Phenotype>& FilterDockWidget::phenotypes() const
{
	return phenotypes_;
}

void FilterDockWidget::setPhenotypes(const QList<Phenotype>& phenotypes)
{
	phenotypes_ = phenotypes;
	phenotypesChanged();
}

QString FilterDockWidget::referenceSample() const
{
	return ui_.refs->toolTip();
}

const FilterCascade& FilterDockWidget::filters() const
{
	return filters_;
}

void FilterDockWidget::addRoi()
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

void FilterDockWidget::addRoiTemp()
{
	//get file to open
	QString path = Settings::path("path_regions");
	QString filename = QFileDialog::getOpenFileName(this, "Select target region file", path, "BED files (*.bed);;All files (*.*)");
	if (filename=="") return;

	//add to list
	ui_.rois->addItem(QFileInfo(filename).fileName(), Helper::canonicalPath(filename));
}

void FilterDockWidget::removeRoi()
{
	QString filename = ui_.rois->itemData(ui_.rois->currentIndex()).toString();
	if (filename=="") return;

	//update settings
	QStringList rois = Settings::stringList("target_regions");
	rois.removeOne(filename);
	Settings::setStringList("target_regions", rois);

	//update GUI
	loadTargetRegions();
	emit filtersChanged();
}

void FilterDockWidget::roiSelectionChanged(int index)
{
	//delete old completer
	QCompleter* completer_old = ui_.rois->completer();
	if (completer_old!=nullptr)
	{
		completer_old->deleteLater();
	}

	//create completer for search mode
	if (ui_.rois->currentIndex()==0)
	{
		ui_.rois->setEditable(true);

		QCompleter* completer = new QCompleter(ui_.rois->model(), ui_.rois);
		completer->setCompletionMode(QCompleter::PopupCompletion);
		completer->setCaseSensitivity(Qt::CaseInsensitive);
		completer->setFilterMode(Qt::MatchContains);
		completer->setCompletionRole(Qt::DisplayRole);
		ui_.rois->setCompleter(completer);
	}
	else
	{
		ui_.rois->setEditable(false);
	}


	ui_.rois->setToolTip(ui_.rois->itemData(index).toString());

	if(index!=0)
	{
		emit filtersChanged();
		emit targetRegionChanged();
	}
}


void FilterDockWidget::referenceSampleChanged(int index)
{
	ui_.refs->setToolTip(ui_.refs->itemData(index).toString());
}

void FilterDockWidget::geneChanged()
{
	if (genes()!=last_genes_)
	{
		last_genes_ = genes();
		emit filtersChanged();
	}
}

void FilterDockWidget::textChanged()
{
	emit filtersChanged();
}

void FilterDockWidget::regionChanged()
{
	emit filtersChanged();
}

void FilterDockWidget::phenotypesChanged()
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

void FilterDockWidget::onFilterCascadeChange(bool update_name)
{
	if (update_name)
	{
		QString name = ui_.filter_name->text();
		if (name!="[none]" && !name.endsWith(" [modified]"))
		{
			ui_.filter_name->setText(name + " [modified]");
		}
	}
	
	emit filterCascadeChanged();
	emit filtersChanged();
}

void FilterDockWidget::showTargetRegionDetails()
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

void FilterDockWidget::updateGeneWarning()
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

void FilterDockWidget::editPhenotypes()
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

void FilterDockWidget::showPhenotypeContextMenu(QPoint pos)
{
	//set up
	QMenu menu;
	if (Settings::boolean("NGSD_enabled", true))
	{
		menu.addAction("load from NGSD");
	}
	if (!phenotypes_.isEmpty())
	{
		menu.addAction("create sub-panel");
		menu.addSeparator();
		menu.addAction("clear");
	}

	//exec
	QAction* action = menu.exec(ui_.hpo_terms->mapToGlobal(pos));
	if (action==nullptr)
	{
		return;
	}
	else if (action->text()=="clear")
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

void FilterDockWidget::updateGUI()
{
	ui_.filters_entries->clear();
	for(int i=0; i<filters_.count(); ++i)
	{
		//remove HTML special characters for GUI
		QTextDocument converter;
		converter.setHtml(filters_[i]->toText());
		QString text = converter.toPlainText();

		QListWidgetItem* item = new QListWidgetItem(text);
		item->setCheckState(filters_[i]->enabled() ? Qt::Checked : Qt::Unchecked);
		item->setToolTip(filters_[i]->description().join("\n"));
		ui_.filters_entries->addItem(item);
	}
	filterSelectionChanged();
}

void FilterDockWidget::filterSelectionChanged()
{
	int index = currentFilterIndex();
	ui_.filter_delete->setEnabled(index!=-1);
	ui_.filter_edit->setEnabled(index!=-1 && filters_[index]->parameters().count()>0);
	ui_.filter_up->setEnabled(index>0);
	ui_.filter_down->setEnabled(index!=-1 && index!=ui_.filters_entries->count()-1);
}

void FilterDockWidget::addFilter()
{
	//show filter menu
	QMenu menu;
	foreach(QString filter_name, FilterFactory::filterNames())
	{
		menu.addAction(filter_name);
	}
	QAction* action = menu.exec(QCursor::pos());
	if(action==nullptr) return;

	//create filter
	QSharedPointer<FilterBase> filter = FilterFactory::create(action->text());
	if (filter->name()=="Filter columns")
	{
		filter->overrideConstraint("entries", "valid", valid_filter_entries_.join(','));
	}

	//determine if filter should be added
	bool add_filter = false;
	if (filter->parameters().count()>0)
	{
		FilterEditDialog dlg(filter, this);
		add_filter = dlg.exec()==QDialog::Accepted;
	}
	else
	{
		add_filter = true;
	}

	//add filter
	if (add_filter)
	{
		filters_.add(filter);
		updateGUI();
		focusFilter(filters_.count()-1);
		onFilterCascadeChange(true);
	}
}

void FilterDockWidget::editSelectedFilter()
{
	int index = currentFilterIndex();
	if (index==-1) return;

	//no paramters => no edit dialog
	if (filters_[index]->parameters().count()==0) return;

	FilterEditDialog dlg(filters_[index], this);
	if (dlg.exec()==QDialog::Accepted)
	{
		updateGUI();
		focusFilter(index);
		onFilterCascadeChange(true);
	}
}

void FilterDockWidget::deleteSelectedFilter()
{
	int index = currentFilterIndex();
	if (index==-1) return;

	filters_.removeAt(index);
	updateGUI();
	focusFilter(index);

	onFilterCascadeChange(true);
}

void FilterDockWidget::moveUpSelectedFilter()
{
	int index = currentFilterIndex();
	if (index==-1) return;

	filters_.moveUp(index);
	updateGUI();
	focusFilter(index-1);

	onFilterCascadeChange(true);
}

void FilterDockWidget::moveDownSelectedFilter()
{
	int index = currentFilterIndex();
	if (index==-1) return;

	filters_.moveDown(index);
	updateGUI();
	focusFilter(index+1);

	onFilterCascadeChange(true);
}

void FilterDockWidget::toggleSelectedFilter(QListWidgetItem* item)
{
	//determine item index
	int index = ui_.filters_entries->row(item);
	if (index==-1) return;

	//check that check state changed (this slot is called for every change of an item)
	if ( (item->checkState()==Qt::Checked && !filters_[index]->enabled()) || (item->checkState()==Qt::Unchecked && filters_[index]->enabled()))
	{
		filters_[index]->toggleEnabled();

		onFilterCascadeChange(true);
	}
}

void FilterDockWidget::addRef()
{
	//get file to open
	QString path = Settings::path("path_variantlists");
	QString filename = QFileDialog::getOpenFileName(this, "Select reference file", path, "BAM files (*.bam);;All files (*.*)");
	if (filename=="") return;

	//get name
	QString name = QInputDialog::getText(this, "Reference file name", "Display name:");
	if (name=="") return;

	//update settings
	QStringList refs = Settings::stringList("reference_files");
	refs.append(name + "\t" + filename);
	refs.sort(Qt::CaseInsensitive);
	refs.removeDuplicates();
	Settings::setStringList("reference_files", refs);

	//update GUI
	loadReferenceFiles();
}

void FilterDockWidget::removeRef()
{
	QString name = ui_.refs->itemText(ui_.refs->currentIndex());
	QString filename = ui_.refs->itemData(ui_.refs->currentIndex()).toString();
	if (filename=="") return;

	//update settings
	QStringList refs = Settings::stringList("reference_files");
	refs.removeOne(name + "\t" + filename);
	Settings::setStringList("reference_files", refs);

	//update GUI
	loadReferenceFiles();
}
