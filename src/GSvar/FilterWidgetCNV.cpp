#include "FilterWidgetCNV.h"
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

FilterWidgetCNV::FilterWidgetCNV(QWidget *parent)
	: QWidget(parent)
	, ui_()
	, filter_widget_(nullptr)
{
	ui_.setupUi(this);

	connect(ui_.roi, SIGNAL(currentIndexChanged(int)), this, SLOT(roiSelectionChanged(int)));

	connect(ui_.gene, SIGNAL(editingFinished()), this, SLOT(geneChanged()));
	connect(ui_.text, SIGNAL(editingFinished()), this, SLOT(textChanged()));
	connect(ui_.region, SIGNAL(editingFinished()), this, SLOT(regionChanged()));
	connect(ui_.f_regs, SIGNAL(valueChanged(int)), this, SIGNAL(filtersChanged()));
	connect(ui_.f_size, SIGNAL(valueChanged(double)), this, SIGNAL(filtersChanged()));

	connect(ui_.hpo, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(showPhenotypeContextMenu(QPoint)));
	connect(ui_.hpo_import, SIGNAL(clicked(bool)), this, SLOT(importHPO()));
	connect(ui_.roi_import, SIGNAL(clicked(bool)), this, SLOT(importROI()));
	connect(ui_.region_import, SIGNAL(clicked(bool)), this, SLOT(importRegion()));
	connect(ui_.gene_import, SIGNAL(clicked(bool)), this, SLOT(importGene()));
	connect(ui_.text_import, SIGNAL(clicked(bool)), this, SLOT(importText()));

	if (Settings::boolean("NGSD_enabled", true))
	{
		connect(ui_.hpo, SIGNAL(clicked(QPoint)), this, SLOT(editPhenotypes()));
	}
	else
	{
		ui_.hpo->setEnabled(false);
	}

	loadTargetRegions();
	reset(true);
}

void FilterWidgetCNV::setVariantFilterWidget(FilterDockWidget* filter_widget)
{
	filter_widget_ = filter_widget;
}

double FilterWidgetCNV::minSizeKb() const
{
	return ui_.f_size->value();
}

int FilterWidgetCNV::minRegs() const
{
	return ui_.f_regs->value();
}

void FilterWidgetCNV::loadTargetRegions()
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

void FilterWidgetCNV::resetSignalsUnblocked(bool clear_roi)
{
	//filter cols
	ui_.f_regs->setValue(1);
	ui_.f_size->setValue(0.0);

    //rois
	if (clear_roi)
	{
		ui_.roi->setCurrentIndex(1);
		ui_.roi->setToolTip("");
	}

    //gene
    last_genes_.clear();
    ui_.gene->clear();
	ui_.text->clear();
	ui_.region->clear();

	//phenotype
	phenotypes_.clear();

	//update GUI
	phenotypesChanged();
}

void FilterWidgetCNV::reset(bool clear_roi)
{
	blockSignals(true);
	resetSignalsUnblocked(clear_roi);
	blockSignals(false);

	if (clear_roi) emit targetRegionChanged();
}

QString FilterWidgetCNV::targetRegion() const
{
	return ui_.roi->toolTip();
}

void FilterWidgetCNV::setTargetRegion(QString roi_file)
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

GeneSet FilterWidgetCNV::genes() const
{
	return GeneSet::createFromText(ui_.gene->text().toLatin1(), ',');
}

QByteArray FilterWidgetCNV::text() const
{
	return ui_.text->text().trimmed().toLatin1();
}

QString FilterWidgetCNV::region() const
{
	return ui_.region->text().trimmed();
}

void FilterWidgetCNV::setRegion(QString region)
{
	ui_.region->setText(region);
	regionChanged();
}

const QList<Phenotype>& FilterWidgetCNV::phenotypes() const
{
	return phenotypes_;
}

void FilterWidgetCNV::setPhenotypes(const QList<Phenotype>& phenotypes)
{
	phenotypes_ = phenotypes;
	phenotypesChanged();
}

void FilterWidgetCNV::roiSelectionChanged(int index)
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

void FilterWidgetCNV::geneChanged()
{
	if (genes()!=last_genes_)
	{
		last_genes_ = genes();
		emit filtersChanged();
	}
}

void FilterWidgetCNV::textChanged()
{
	emit filtersChanged();
}

void FilterWidgetCNV::regionChanged()
{
	emit filtersChanged();
}

void FilterWidgetCNV::phenotypesChanged()
{
	//update GUI
	QByteArrayList tmp;
	foreach(const Phenotype& pheno, phenotypes_)
	{
		tmp << pheno.name();
	}

	ui_.hpo->setText(tmp.join("; "));

	QString tooltip = "Phenotype/inheritance filter based on HPO terms.<br><br>Notes:<br>- This functionality is only available when NGSD is enabled.<br>- Filters based on the phenotype-associated gene loci including 5000 flanking bases.";
	if (!phenotypes_.isEmpty())
	{
		tooltip += "<br><br><nobr>Currently selected HPO terms:</nobr>";
		foreach(const Phenotype& pheno, phenotypes_)
		{
			tooltip += "<br><nobr>" + pheno.toString() + "</nobr>";
		}
	}
	ui_.hpo->setToolTip(tooltip);

	emit filtersChanged();
}


void FilterWidgetCNV::editPhenotypes()
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

void FilterWidgetCNV::showPhenotypeContextMenu(QPoint pos)
{
	//set up
	QMenu menu;
	menu.addAction("clear");

	//exec
	QAction* action = menu.exec(ui_.hpo->mapToGlobal(pos));
	if (action==nullptr) return;

	if (action->text()=="clear")
	{
		phenotypes_.clear();
		phenotypesChanged();
	}
}

void FilterWidgetCNV::importHPO()
{
	setPhenotypes(filter_widget_->phenotypes());
	phenotypesChanged();
}

void FilterWidgetCNV::importROI()
{
	ui_.roi->setCurrentText(filter_widget_->targetRegionName());
	emit filtersChanged();
}

void FilterWidgetCNV::importRegion()
{
	ui_.region->setText(filter_widget_->region());
	emit filtersChanged();
}

void FilterWidgetCNV::importGene()
{
	ui_.gene->setText(filter_widget_->genes().join(", "));
	emit filtersChanged();
}

void FilterWidgetCNV::importText()
{
	ui_.text->setText(filter_widget_->text());
	emit filtersChanged();
}
