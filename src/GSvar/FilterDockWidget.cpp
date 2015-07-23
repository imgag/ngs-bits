#include "FilterDockWidget.h"
#include "Settings.h"
#include <QCheckBox>
#include <QFileInfo>
#include <QFileDialog>
#include <QInputDialog>

FilterDockWidget::FilterDockWidget(QWidget *parent)
	: QDockWidget(parent)
	, ui_()
{
	ui_.setupUi(this);

	connect(ui_.maf, SIGNAL(valueChanged(double)), this, SIGNAL(filtersChanged()));
	connect(ui_.maf_enabled, SIGNAL(toggled(bool)), this, SIGNAL(filtersChanged()));

	connect(ui_.impact, SIGNAL(currentIndexChanged(int)), this, SIGNAL(filtersChanged()));
	connect(ui_.impact_enabled, SIGNAL(toggled(bool)), this, SIGNAL(filtersChanged()));

	connect(ui_.ihdb, SIGNAL(valueChanged(int)), this, SIGNAL(filtersChanged()));
	connect(ui_.ihdb_enabled, SIGNAL(toggled(bool)), this, SIGNAL(filtersChanged()));

	connect(ui_.vus, SIGNAL(currentIndexChanged(int)), this, SIGNAL(filtersChanged()));
	connect(ui_.vus_enabled, SIGNAL(toggled(bool)), this, SIGNAL(filtersChanged()));

	connect(ui_.geno, SIGNAL(currentIndexChanged(int)), this, SIGNAL(filtersChanged()));
	connect(ui_.geno_enabled, SIGNAL(toggled(bool)), this, SIGNAL(filtersChanged()));

	connect(ui_.quality_enabled, SIGNAL(toggled(bool)), this, SIGNAL(filtersChanged()));
	connect(ui_.trio_enabled, SIGNAL(toggled(bool)), this, SIGNAL(filtersChanged()));
	connect(ui_.important_enabled, SIGNAL(toggled(bool)), this, SIGNAL(filtersChanged()));

	connect(ui_.roi_add, SIGNAL(clicked()), this, SLOT(addRoi()));
	connect(ui_.roi_add_temp, SIGNAL(clicked()), this, SLOT(addRoiTemp()));
	connect(ui_.roi_remove, SIGNAL(clicked()), this, SLOT(removeRoi()));
	connect(ui_.rois, SIGNAL(currentIndexChanged(int)), this, SLOT(roiSelectionChanged(int)));

	connect(ui_.ref_add, SIGNAL(clicked()), this, SLOT(addRef()));
	connect(ui_.ref_remove, SIGNAL(clicked()), this, SLOT(removeRef()));
	connect(ui_.refs, SIGNAL(currentIndexChanged(int)), this, SLOT(referenceSampleChanged(int)));

	connect(ui_.gene, SIGNAL(editingFinished()), this, SLOT(geneChanged()));

	loadROIFilters();
	loadReferenceFiles();

	reset();
}

void FilterDockWidget::loadROIFilters()
{
	ui_.rois->blockSignals(true);

	//store old selection
	QString current = ui_.rois->currentText();

	//load from settings
	ui_.rois->clear();
	ui_.rois->addItem("none", "");
	QStringList rois = Settings::stringList("target_regions");
	rois.sort(); //note: path is included in order!
	foreach(const QString& roi_file, rois)
	{
		QFileInfo info(roi_file);
		ui_.rois->addItem(info.fileName(), roi_file);
	}

	//restore old selection
	int current_index = ui_.rois->findText(current);
	if (current_index==-1) current_index = 0;
	ui_.rois->setCurrentIndex(current_index);

	ui_.rois->blockSignals(false);
}


void FilterDockWidget::loadReferenceFiles()
{
	//store old selection
	QString current = ui_.rois->currentText();

	//load from settings
	ui_.refs->clear();
	ui_.refs->addItem("none", "");
	QStringList refs = Settings::stringList("reference_files");
	foreach(const QString& roi_file, refs)
	{
		QStringList parts = roi_file.split("\t");
		ui_.refs->addItem(parts[0], parts[1]);
	}

	//restore old selection
	int current_index = ui_.refs->findText(current);
	if (current_index==-1) current_index = 0;
	ui_.refs->setCurrentIndex(current_index);
}

void FilterDockWidget::reset()
{
	blockSignals(true);

	//annotations
	ui_.maf_enabled->setChecked(false);
	ui_.maf->setValue(1.0);
	ui_.impact_enabled->setChecked(false);
	ui_.impact->setCurrentText("HIGH,MODERATE,LOW");
	ui_.ihdb_enabled->setChecked(false);
	ui_.ihdb->setValue(5);
	ui_.vus_enabled->setChecked(false);
	ui_.vus->setCurrentText("3");
	ui_.geno_enabled->setChecked(false);
	ui_.geno->setCurrentText("hom");
	ui_.important_enabled->setChecked(false);
	ui_.quality_enabled->setChecked(false);
	ui_.trio_enabled->setChecked(false);

	//rois
	ui_.rois->setCurrentIndex(0);
	ui_.rois->setToolTip("");

	//refs
	ui_.refs->setCurrentIndex(0);
	ui_.refs->setToolTip("");

	//gene
	last_gene_ = "";
	ui_.gene->clear();

	blockSignals(false);
}

void FilterDockWidget::applyDefaultFilters()
{
	//block signals to avoid 10 updates of GUI
	blockSignals(true);

	//enable default filters
	ui_.maf_enabled->setChecked(true);
	ui_.maf->setValue(1.0);
	ui_.impact_enabled->setChecked(true);
	ui_.impact->setCurrentText("HIGH,MODERATE,LOW");
	ui_.ihdb_enabled->setChecked(true);
	ui_.ihdb->setValue(5);
	ui_.vus_enabled->setChecked(true);
	ui_.vus->setCurrentText("3");
	ui_.geno_enabled->setChecked(false);
	ui_.geno->setCurrentText("hom");
	ui_.important_enabled->setChecked(true);
	ui_.quality_enabled->setChecked(false);
	ui_.trio_enabled->setChecked(false);

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

bool FilterDockWidget::applyImpact() const
{
	return ui_.impact_enabled->isChecked();
}

QStringList FilterDockWidget::impact() const
{
	return ui_.impact->currentText().split(",");
}

bool FilterDockWidget::applyVus() const
{
	return ui_.vus_enabled->isChecked();
}

int FilterDockWidget::vus() const
{
	return ui_.vus->currentText().toInt();
}

bool FilterDockWidget::applyGenotype() const
{
	return ui_.geno_enabled->isChecked();
}

QString FilterDockWidget::genotype() const
{
	return ui_.geno->currentText();
}

bool FilterDockWidget::applyIhdb() const
{
	return ui_.ihdb_enabled->isChecked();
}

int FilterDockWidget::ihdb() const
{
	return ui_.ihdb->value();
}

bool FilterDockWidget::applyQuality() const
{
	return ui_.quality_enabled->isChecked();
}

bool FilterDockWidget::applyTrio() const
{
	return ui_.trio_enabled->isChecked();
}

bool FilterDockWidget::keepImportant() const
{
	return ui_.important_enabled->isChecked();
}

QString FilterDockWidget::targetRegion() const
{
	return ui_.rois->toolTip();
}

QString FilterDockWidget::gene() const
{
return ui_.gene->text();
}

QString FilterDockWidget::referenceSample() const
{
	return ui_.refs->toolTip();
}

QStringList FilterDockWidget::appliedFilters() const
{
	QStringList output;
	if (applyMaf()) output.append("maf<=" + QString::number(mafPerc(), 'f', 2) + "%");
	if (applyImpact()) output.append("impact=" + impact().join(","));
	if (applyIhdb()) output.append("ihdb<" + QString::number(ihdb()));
	if (applyVus()) output.append("classification>=" + QString::number(vus()));
	if (applyGenotype()) output.append("genotype=" + genotype());
	if (keepImportant()) output.append("keep_important");
	if (applyQuality()) output.append("quality");
	if (applyTrio()) output.append("trio");

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
	loadROIFilters();
}

void FilterDockWidget::addRoiTemp()
{
	//get file to open
	QString path = Settings::path("path_regions");
	QString filename = QFileDialog::getOpenFileName(this, "Select target region file", path, "BED files (*.bed);;All files (*.*)");
	if (filename=="") return;

	//add to list
	ui_.rois->addItem(QFileInfo(filename).fileName(), filename);
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
	loadROIFilters();
	emit filtersChanged();
}

void FilterDockWidget::roiSelectionChanged(int index)
{
	ui_.rois->setToolTip(ui_.rois->itemData(index).toString());

	emit filtersChanged();
}


void FilterDockWidget::referenceSampleChanged(int index)
{
	ui_.refs->setToolTip(ui_.refs->itemData(index).toString());
}

void FilterDockWidget::geneChanged()
{
	if (gene()!=last_gene_)
	{
		last_gene_ = gene();
		emit filtersChanged();
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
