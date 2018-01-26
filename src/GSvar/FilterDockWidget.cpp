#include "FilterDockWidget.h"
#include "Settings.h"
#include "Helper.h"
#include "FilterColumnWidget.h"
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

FilterDockWidget::FilterDockWidget(QWidget *parent)
	: QDockWidget(parent)
	, ui_()
	, ngsd_enabled(Settings::boolean("NGSD_enabled", true))
{
	ui_.setupUi(this);

	connect(ui_.maf, SIGNAL(valueChanged(double)), this, SIGNAL(filtersChanged()));
	connect(ui_.maf_enabled, SIGNAL(toggled(bool)), this, SIGNAL(filtersChanged()));

	connect(ui_.maf_sub, SIGNAL(valueChanged(double)), this, SIGNAL(filtersChanged()));
	connect(ui_.maf_sub_enabled, SIGNAL(toggled(bool)), this, SIGNAL(filtersChanged()));

	connect(ui_.impact, SIGNAL(currentIndexChanged(int)), this, SIGNAL(filtersChanged()));
	connect(ui_.impact_enabled, SIGNAL(toggled(bool)), this, SIGNAL(filtersChanged()));

	connect(ui_.ihdb, SIGNAL(valueChanged(int)), this, SIGNAL(filtersChanged()));
	connect(ui_.ihdb_enabled, SIGNAL(toggled(bool)), this, SIGNAL(filtersChanged()));
	connect(ui_.ihdb_ignore_gt, SIGNAL(toggled(bool)), this, SIGNAL(filtersChanged()));

	connect(ui_.classification, SIGNAL(currentIndexChanged(int)), this, SIGNAL(filtersChanged()));
	connect(ui_.classification_enabled, SIGNAL(toggled(bool)), this, SIGNAL(filtersChanged()));

	connect(ui_.geno_affected, SIGNAL(currentIndexChanged(int)), this, SIGNAL(filtersChanged()));
	connect(ui_.geno_affected_enabled, SIGNAL(toggled(bool)), this, SIGNAL(filtersChanged()));
	connect(ui_.geno_control, SIGNAL(currentIndexChanged(int)), this, SIGNAL(filtersChanged()));
	connect(ui_.geno_control_enabled, SIGNAL(toggled(bool)), this, SIGNAL(filtersChanged()));

	connect(ui_.keep_class_ge_enabled, SIGNAL(toggled(bool)), this, SIGNAL(filtersChanged()));
	connect(ui_.keep_class_ge, SIGNAL(currentTextChanged(QString)), this, SIGNAL(filtersChanged()));
	connect(ui_.keep_class_m, SIGNAL(toggled(bool)), this, SIGNAL(filtersChanged()));

	connect(ui_.gene_pli, SIGNAL(valueChanged(double)), this, SIGNAL(filtersChanged()));
	connect(ui_.gene_pli_enabled, SIGNAL(toggled(bool)), this, SIGNAL(filtersChanged()));

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
	connect(ui_.region, SIGNAL(editingFinished()), this, SLOT(regionChanged()));

	if (ngsd_enabled)
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

	reset(true, true);
}

void FilterDockWidget::setFilterColumns(const QMap<QString, QString>& filter_cols)
{
	//remove old widgets
	QLayoutItem* item;
	while ((item = ui_.filter_col->layout()->takeAt(0)) != nullptr)
	{
		delete item->widget();
		delete item;
	}

	//add new widgets
	auto it = filter_cols.cbegin();
	while(it!=filter_cols.cend())
	{
		auto w = new FilterColumnWidget(it.key(), it.value());
		connect(w, SIGNAL(stateChanged()), this, SLOT(filterColumnStateChanged()));
		ui_.filter_col->layout()->addWidget(w);

		++it;
	}
	ui_.filter_col->layout()->addItem(new QSpacerItem(1,1, QSizePolicy::Minimum, QSizePolicy::MinimumExpanding));
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

void FilterDockWidget::resetSignalsUnblocked(bool clear_roi, bool clear_off_target)
{
    //annotations
    ui_.maf_enabled->setChecked(false);
    ui_.maf->setValue(1.0);
	ui_.maf_sub_enabled->setChecked(false);
	ui_.maf_sub->setValue(1.0);
    ui_.impact_enabled->setChecked(false);
    ui_.impact->setCurrentText("HIGH,MODERATE,LOW");
    ui_.ihdb_enabled->setChecked(false);
	ui_.ihdb->setValue(20);
	ui_.ihdb_ignore_gt->setChecked(false);
	ui_.classification_enabled->setChecked(false);
    ui_.classification->setCurrentText("3");
	ui_.geno_affected_enabled->setChecked(false);
	ui_.geno_affected->setCurrentText("hom");
	ui_.geno_control_enabled->setChecked(false);
	ui_.geno_control->setCurrentText("wt");
    ui_.keep_class_ge_enabled->setChecked(false);
    ui_.keep_class_ge->setCurrentText("3");
	ui_.keep_class_m->setChecked(false);
	ui_.gene_pli_enabled->setChecked(false);
	ui_.gene_pli->setValue(0.9);

    //filter cols
    QList<FilterColumnWidget*> fcws = ui_.filter_col->findChildren<FilterColumnWidget*>();
    foreach(FilterColumnWidget* w, fcws)
    {
        w->setState(FilterColumnWidget::NONE);
        w->setFilter(false);

		//disable off-target by default
		if (w->objectName()=="off-target")
		{
			if (clear_off_target)
			{
				w->setState(FilterColumnWidget::NONE);
			}
			else
			{
				w->setState(FilterColumnWidget::REMOVE);
			}
		}
    }

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
	ui_.region->clear();

	//phenotype
	phenotypes_.clear();
	phenotypesChanged();

    //refs
    ui_.refs->setCurrentIndex(0);
    ui_.refs->setToolTip("");
}

void FilterDockWidget::reset(bool clear_roi, bool clear_off_target)
{
	blockSignals(true);
	resetSignalsUnblocked(clear_roi, clear_off_target);
	blockSignals(false);

    emit filtersChanged();
	if (clear_roi) emit targetRegionChanged();
}

void FilterDockWidget::applyDefaultFilters()
{
	//block signals to avoid 10 updates of GUI
	blockSignals(true);

	resetSignalsUnblocked(false, true);

	//enable default filters
	ui_.maf_enabled->setChecked(true);
	ui_.maf->setValue(1.0);
	ui_.maf_sub_enabled->setChecked(true);
	ui_.maf_sub->setValue(1.0);
	ui_.impact_enabled->setChecked(true);
	ui_.impact->setCurrentText("HIGH,MODERATE,LOW");
	ui_.ihdb_enabled->setChecked(true);
	ui_.ihdb->setValue(20);
	ui_.ihdb_ignore_gt->setChecked(false);
	ui_.classification_enabled->setChecked(true);
	ui_.classification->setCurrentText("3");
    ui_.keep_class_ge_enabled->setChecked(true);
	ui_.keep_class_ge->setCurrentText("3");
	ui_.keep_class_m->setChecked(true);

	//filter cols
	QList<FilterColumnWidget*> fcws = ui_.filter_col->findChildren<FilterColumnWidget*>();
	foreach(FilterColumnWidget* w, fcws)
	{
		if (w->objectName()=="anno_high_impact" || w->objectName()=="anno_pathogenic_clinvar" || w->objectName()=="anno_pathogenic_hgmd")
		{
			w->setState(FilterColumnWidget::KEEP);
		}
		else if (w->objectName()=="off-target")
		{
			w->setState(FilterColumnWidget::REMOVE);
		}
	}

	//re-enable signals
	blockSignals(false);

	//emit signal to update GUI
	emit filtersChanged();
}

void FilterDockWidget::applyDefaultFiltersRecessive()
{
	//block signals to avoid 10 updates of GUI
	blockSignals(true);

	resetSignalsUnblocked(false, true);

	//enable default filters
	ui_.maf_enabled->setChecked(true);
	ui_.maf->setValue(1.0);
	ui_.maf_sub_enabled->setChecked(true);
	ui_.maf_sub->setValue(1.0);
	ui_.impact_enabled->setChecked(true);
	ui_.impact->setCurrentText("HIGH,MODERATE,LOW");
	ui_.ihdb_enabled->setChecked(true);
	ui_.ihdb->setValue(15);
	ui_.ihdb_ignore_gt->setChecked(true);
	ui_.classification_enabled->setChecked(false);
	ui_.classification->setCurrentText("3");
	ui_.keep_class_ge_enabled->setChecked(true);
	ui_.keep_class_ge->setCurrentText("4");
	ui_.keep_class_m->setChecked(false);
	ui_.geno_affected_enabled->setChecked(true);
	ui_.geno_affected->setCurrentText("compound-het or hom");

	//filter cols
	QList<FilterColumnWidget*> fcws = ui_.filter_col->findChildren<FilterColumnWidget*>();
	foreach(FilterColumnWidget* w, fcws)
	{
		if (w->objectName()=="gene_blacklist")
		{
			w->setState(FilterColumnWidget::REMOVE);
		}
	}

	//re-enable signals
	blockSignals(false);

	//emit signal to update GUI
	emit filtersChanged();
}

void FilterDockWidget::applyDefaultFiltersTrio()
{
	//block signals to avoid 10 updates of GUI
	blockSignals(true);

	resetSignalsUnblocked(false, true);

	//enable default filters
	ui_.maf_enabled->setChecked(true);
	ui_.maf->setValue(1.0);
	ui_.maf_sub_enabled->setChecked(true);
	ui_.maf_sub->setValue(1.0);
	ui_.impact_enabled->setChecked(true);
	ui_.impact->setCurrentText("HIGH,MODERATE,LOW");
	ui_.ihdb_enabled->setChecked(true);
	ui_.ihdb->setValue(20);
	ui_.ihdb_ignore_gt->setChecked(false);
	ui_.keep_class_ge_enabled->setChecked(true);
	ui_.keep_class_ge->setCurrentText("4");

	//filter cols
	QList<FilterColumnWidget*> fcws = ui_.filter_col->findChildren<FilterColumnWidget*>();
	foreach(FilterColumnWidget* w, fcws)
	{
		if (w->objectName().startsWith("trio_"))
		{
			w->setState(FilterColumnWidget::KEEP);
			w->setFilter(true);
		}
	}

	//re-enable signals
	blockSignals(false);

	//emit signal to update GUI
	emit filtersChanged();
}

void FilterDockWidget::applyDefaultFiltersMultiSample()
{
	//block signals to avoid 10 updates of GUI
	blockSignals(true);

	resetSignalsUnblocked(false, true);

	//enable default filters
	ui_.maf_enabled->setChecked(true);
	ui_.maf->setValue(1.0);
	ui_.maf_sub_enabled->setChecked(true);
	ui_.maf_sub->setValue(1.0);
	ui_.impact_enabled->setChecked(true);
	ui_.impact->setCurrentText("HIGH,MODERATE,LOW");
	ui_.ihdb_enabled->setChecked(true);
	ui_.ihdb->setValue(20);
	ui_.ihdb_ignore_gt->setChecked(false);
	ui_.classification_enabled->setChecked(true);
	ui_.classification->setCurrentText("3");
	ui_.keep_class_ge_enabled->setChecked(true);
	ui_.keep_class_ge->setCurrentText("3");
	ui_.keep_class_m->setChecked(true);

	//filter cols
	QList<FilterColumnWidget*> fcws = ui_.filter_col->findChildren<FilterColumnWidget*>();
	foreach(FilterColumnWidget* w, fcws)
	{
		if (w->objectName()=="anno_pathogenic_clinvar" || w->objectName()=="anno_pathogenic_hgmd")
		{
			w->setState(FilterColumnWidget::KEEP);
		}
		else if (w->objectName()=="off-target")
		{
			w->setState(FilterColumnWidget::REMOVE);
		}
	}

	//re-enable signals
	blockSignals(false);

	//emit signal to update GUI
	emit filtersChanged();
}

void FilterDockWidget::applyDefaultFiltersSomatic()
{
    //block signals to avoid 10 updates of GUI
    blockSignals(true);

	resetSignalsUnblocked(false, true);

    //enable default filters
	ui_.maf_enabled->setChecked(false);
    ui_.maf->setValue(1.0);
	ui_.maf_sub_enabled->setChecked(false);
	ui_.maf_sub->setValue(1.0);

    //filter cols
    QList<FilterColumnWidget*> fcws = ui_.filter_col->findChildren<FilterColumnWidget*>();
    foreach(FilterColumnWidget* w, fcws)
	{
		w->setState(FilterColumnWidget::REMOVE);
	}

    //re-enable signals
    blockSignals(false);

    //emit signal to update GUI
    emit filtersChanged();
}

void FilterDockWidget::applyDefaultFiltersCarrier()
{
	//block signals to avoid 10 updates of GUI
	blockSignals(true);

	resetSignalsUnblocked(false, true);

	//enable default filters
	ui_.maf_enabled->setChecked(true);
	ui_.maf->setValue(1.0);
	ui_.maf_sub_enabled->setChecked(true);
	ui_.maf_sub->setValue(1.0);
	ui_.impact_enabled->setChecked(true);
	ui_.impact->setCurrentText("HIGH");
	ui_.ihdb_enabled->setChecked(true);
	ui_.ihdb->setValue(50);
	ui_.ihdb_ignore_gt->setChecked(false);
	ui_.classification_enabled->setChecked(true);
	ui_.classification->setCurrentText("4");
	ui_.keep_class_ge_enabled->setChecked(true);
	ui_.keep_class_ge->setCurrentText("4");
	ui_.keep_class_m->setChecked(false);

	//filter cols
	QList<FilterColumnWidget*> fcws = ui_.filter_col->findChildren<FilterColumnWidget*>();
	foreach(FilterColumnWidget* w, fcws)
	{
		if (w->objectName()=="anno_omim")
		{
			w->setFilter(true);
		}
		if (w->objectName()=="anno_pathogenic_clinvar" || w->objectName()=="anno_pathogenic_hgmd")
		{
			w->setState(FilterColumnWidget::KEEP);
		}
	}

	//re-enable signals
	blockSignals(false);

	//emit signal to update GUI
	emit filtersChanged();
}

bool FilterDockWidget::applyMaf() const
{
	return ui_.maf_enabled->isChecked();
}

double FilterDockWidget::mafPerc() const
{
	return ui_.maf->value();
}

bool FilterDockWidget::applyMafSub() const
{
	return ui_.maf_sub_enabled->isChecked();
}

double FilterDockWidget::mafSubPerc() const
{
	return ui_.maf_sub->value();
}

bool FilterDockWidget::applyImpact() const
{
	return ui_.impact_enabled->isChecked();
}

QStringList FilterDockWidget::impact() const
{
	return ui_.impact->currentText().split(",");
}

bool FilterDockWidget::applyClassification() const
{
	return ui_.classification_enabled->isChecked();
}

int FilterDockWidget::classification() const
{
	return ui_.classification->currentText().toInt();
}

bool FilterDockWidget::applyGenotypeAffected() const
{
	return ui_.geno_affected_enabled->isChecked();
}

QString FilterDockWidget::genotypeAffected() const
{
	return ui_.geno_affected->currentText();
}

bool FilterDockWidget::applyGenotypeControl() const
{
	return ui_.geno_control_enabled->isChecked();
}

QString FilterDockWidget::genotypeControl() const
{
	return ui_.geno_control->currentText();
}

bool FilterDockWidget::applyIhdb() const
{
	return ui_.ihdb_enabled->isChecked();
}

int FilterDockWidget::ihdb() const
{
	return ui_.ihdb->value();
}

int FilterDockWidget::ihdbIgnoreGenotype() const
{
	return ui_.ihdb_ignore_gt->isChecked();
}

int FilterDockWidget::keepClassGreaterEqual() const
{
	if (!ui_.keep_class_ge_enabled->isChecked()) return -1;
	return ui_.keep_class_ge->currentText().toInt();
}

bool FilterDockWidget::keepClassM() const
{
	return ui_.keep_class_m->isChecked();
}

bool FilterDockWidget::applyPLI() const
{
	return ui_.gene_pli_enabled->isChecked();
}

double FilterDockWidget::pli() const
{
	return ui_.gene_pli->value();
}

QStringList FilterDockWidget::filterColumnsKeep() const
{
	QStringList output;
	QList<FilterColumnWidget*> fcws = ui_.filter_col->findChildren<FilterColumnWidget*>();
	foreach(FilterColumnWidget* w, fcws)
	{
		if (w->state()==FilterColumnWidget::KEEP)
		{
			output.append(w->objectName());
		}
	}
	return output;
}

QStringList FilterDockWidget::filterColumnsRemove() const
{
	QStringList output;
	QList<FilterColumnWidget*> fcws = ui_.filter_col->findChildren<FilterColumnWidget*>();
	foreach(FilterColumnWidget* w, fcws)
	{
		if (w->state()==FilterColumnWidget::REMOVE)
		{
			output.append(w->objectName());
		}
	}
    return output;
}

QStringList FilterDockWidget::filterColumnsFilter() const
{
	QStringList output;
    QList<FilterColumnWidget*> fcws = ui_.filter_col->findChildren<FilterColumnWidget*>();
    foreach(FilterColumnWidget* w, fcws)
    {
        if (w->filter())
        {
			output.append(w->objectName());
        }
    }
    return output;
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

QMap<QString, QString> FilterDockWidget::appliedFilters() const
{
	QMap<QString, QString> output;
	if (applyMaf()) output.insert("maf", QString::number(mafPerc(), 'f', 2) + "%");
	if (applyImpact()) output.insert("impact", impact().join(","));
	if (applyIhdb()) output.insert("ihdb", QString::number(ihdb()));
	if (applyClassification()) output.insert("classification", QString::number(classification()));
	if (applyGenotypeAffected()) output.insert("genotype (affected)" , genotypeAffected());
	if (applyGenotypeControl()) output.insert("genotype (control)" , genotypeControl());
	if (keepClassM()) output.insert("keep_class_m", "");
	if (keepClassGreaterEqual()!=-1) output.insert("keep_class_ge", QString::number(keepClassGreaterEqual()));

	return output;
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
	if ((void*)completer_old!=0)
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
	//qDebug() << __LINE__ << ui_.rois->completer() << ui_.rois->itemData(index).toString();
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

	QString tooltip = "Phenotype/inheritance filter based on HPO terms.";
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

void FilterDockWidget::filterColumnStateChanged()
{
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
	bool show_warning = false;

	QString roi = targetRegion();
	if (roi!="")
	{
		QString genes_file = roi.left(roi.size()-4) + "_genes.txt";
		if (QFile::exists(genes_file))
		{
			GeneSet roi_genes = GeneSet::createFromFile(genes_file);
			GeneSet non_coding;
			non_coding << "PADI6" << "SRD5A2" << "SYN2" << "NEFL" << "ABO" << "NR2E3" << "TTC25";
			GeneSet inter = roi_genes.intersect(non_coding);
			if (!inter.isEmpty())
			{
				show_warning = true;
				ui_.gene_warning->setToolTip("Some genes (" + inter.join(", ") + ") of the target region are non-coding in the Ensembl annotation of GRCh37, but coding for GRCh38.\nVariants in non-coding genes have LOW/MODIFIER impact. Make sure to check these variants too!");
			}
		}
	}

	ui_.gene_warning->setHidden(!show_warning);
}

void FilterDockWidget::editPhenotypes()
{
	//edit
	PhenotypeSelectionWidget* selector = new PhenotypeSelectionWidget(this);
	selector->setPhenotypes(phenotypes_);
	auto dlg = GUIHelper::showWidgetAsDialog(selector, "Select HPO terms", true);

	//update
	if (dlg->result()==QDialog::Accepted)
	{
		phenotypes_ = selector->selectedPhenotypes();
		phenotypesChanged();
	}
}

void FilterDockWidget::showPhenotypeContextMenu(QPoint pos)
{
	//set up
	QMenu menu;
	menu.addAction("import from GenLab");
	menu.addSeparator();
	menu.addAction("clear");

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
	else if (action->text()=="import from GenLab")
	{
		emit phenotypeDataImportRequested();
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
