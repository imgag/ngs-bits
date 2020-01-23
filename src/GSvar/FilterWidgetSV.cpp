#include "FilterWidgetSV.h"
#include "Settings.h"
#include "Helper.h"
#include "NGSD.h"
#include "Log.h"
#include "PhenotypeSelectionWidget.h"
#include "GUIHelper.h"
#include "GSvarHelper.h"
#include <QFileInfo>
#include <QCompleter>
#include <QMenu>
#include <QDialog>
#include <QDir>
#include <QMessageBox>

FilterWidgetSV::FilterWidgetSV(QWidget *parent)
	: QWidget(parent)
	, ui_()
	, filter_widget_(nullptr)
{
	ui_.setupUi(this);
	ui_.cascade_widget->setSubject(VariantType::SVS);
	connect(ui_.cascade_widget, SIGNAL(filterCascadeChanged()), this, SLOT(updateFilterName()));
	connect(ui_.cascade_widget, SIGNAL(filterCascadeChanged()), this, SIGNAL(filtersChanged()));
	connect(ui_.filters, SIGNAL(currentIndexChanged(int)), this, SLOT(setFilter(int)));
	ui_.lab_modified->setHidden(true);

	connect(ui_.roi, SIGNAL(currentIndexChanged(int)), this, SLOT(roiSelectionChanged(int)));
	connect(ui_.gene, SIGNAL(editingFinished()), this, SLOT(geneChanged()));
	connect(ui_.text, SIGNAL(editingFinished()), this, SLOT(textChanged()));
	connect(ui_.region, SIGNAL(editingFinished()), this, SLOT(regionChanged()));
	connect(ui_.hpo, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(showPhenotypeContextMenu(QPoint)));

	connect(ui_.hpo_import, SIGNAL(clicked(bool)), this, SLOT(importHPO()));
	connect(ui_.roi_import, SIGNAL(clicked(bool)), this, SLOT(importROI()));
	connect(ui_.region_import, SIGNAL(clicked(bool)), this, SLOT(importRegion()));
	connect(ui_.gene_import, SIGNAL(clicked(bool)), this, SLOT(importGene()));
	connect(ui_.text_import, SIGNAL(clicked(bool)), this, SLOT(importText()));

	QAction* action = new QAction("clear", this);
	connect(action, &QAction::triggered, this, &FilterWidgetSV::clearTargetRegion);
	ui_.roi->addAction(action);

	connect(ui_.hpo, SIGNAL(clicked(QPoint)), this, SLOT(editPhenotypes()));
	ui_.hpo->setEnabled(Settings::boolean("NGSD_enabled", true));

	FilterWidget::loadTargetRegions(ui_.roi);
	loadFilters();
	reset(true);
}

void FilterWidgetSV::setVariantFilterWidget(FilterWidget* filter_widget)
{
	filter_widget_ = filter_widget;
}

void FilterWidgetSV::setValidFilterEntries(const QStringList& filter_entries)
{
	ui_.cascade_widget->setValidFilterEntries(filter_entries);
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
		ui_.roi->setToolTip("");
	}

	//gene
	last_genes_.clear();
	ui_.gene->clear();
	ui_.text->clear();
	ui_.region->clear();

	//phenotype
	phenotypes_.clear();
	phenotypesChanged();
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

	if (clear_roi) emit targetRegionChanged();
}

const FilterCascade& FilterWidgetSV::filters() const
{
	return ui_.cascade_widget->filters();
}

void FilterWidgetSV::markFailedFilters()
{
	ui_.cascade_widget->markFailedFilters();
}

QString FilterWidgetSV::targetRegion() const
{
	return ui_.roi->toolTip();
}

void FilterWidgetSV::setTargetRegion(QString roi_file)
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

GeneSet FilterWidgetSV::genes() const
{
	return GeneSet::createFromText(ui_.gene->text().toLatin1(), ',');
}

QByteArray FilterWidgetSV::text() const
{
	return ui_.text->text().trimmed().toLatin1();
}

QString FilterWidgetSV::region() const
{
	return ui_.region->text().trimmed();
}

void FilterWidgetSV::setRegion(QString region)
{
	ui_.region->setText(region);
	regionChanged();
}

const QList<Phenotype>& FilterWidgetSV::phenotypes() const
{
	return phenotypes_;
}

void FilterWidgetSV::setPhenotypes(const QList<Phenotype>& phenotypes)
{
	phenotypes_ = phenotypes;
	phenotypesChanged();
}


void FilterWidgetSV::roiSelectionChanged(int index)
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

void FilterWidgetSV::geneChanged()
{
	if (genes()!=last_genes_)
	{
		last_genes_ = genes();
		emit filtersChanged();
	}
}

void FilterWidgetSV::textChanged()
{
	emit filtersChanged();
}

void FilterWidgetSV::regionChanged()
{
	emit filtersChanged();
}

void FilterWidgetSV::phenotypesChanged()
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


void FilterWidgetSV::editPhenotypes()
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

void FilterWidgetSV::showPhenotypeContextMenu(QPoint pos)
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

void FilterWidgetSV::importHPO()
{
	setPhenotypes(filter_widget_->phenotypes());
	phenotypesChanged();
}

void FilterWidgetSV::importROI()
{
	ui_.roi->setCurrentText(filter_widget_->targetRegionName());
	emit filtersChanged();
}

void FilterWidgetSV::importRegion()
{
	ui_.region->setText(filter_widget_->region());
	emit filtersChanged();
}

void FilterWidgetSV::importGene()
{
	ui_.gene->setText(filter_widget_->genes().join(", "));
	emit filtersChanged();
}

void FilterWidgetSV::importText()
{
	ui_.text->setText(filter_widget_->text());
	emit filtersChanged();
}

void FilterWidgetSV::updateFilterName()
{
	if (ui_.filters->currentText()=="[none]") return;

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
