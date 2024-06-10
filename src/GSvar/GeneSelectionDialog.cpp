#include "GeneSelectionDialog.h"
#include "PhenotypeSelectionWidget.h"
#include "ui_GeneSelectionDialog.h"

#include "GUIHelper.h"
#include "NGSD.h"
#include "GlobalServiceProvider.h"

GeneSelectionDialog::GeneSelectionDialog(QWidget *parent) :
	QDialog(parent),
	ui_(new Ui::GeneSelectionDialog),
	variant_filter_widget_(nullptr)
{
	ui_->setupUi(this);

	loadTargetRegions();

	connect(ui_->b_apply, SIGNAL(clicked(bool)), this, SLOT(determineGenes()));
	connect(ui_->le_hpo, SIGNAL(clicked(QPoint)), this, SLOT(editPhenotypes()));
}

GeneSelectionDialog::~GeneSelectionDialog()
{
	delete ui_;
}

GeneSet GeneSelectionDialog::geneSet()
{
	GeneSet genes;
	// parse text field
	QByteArray raw_text = ui_->te_genes->toPlainText().replace(" ", "\n").replace("\t", "\n").replace(",", "\n").replace(";", "\n").replace("\r", "").toUtf8();
	genes.insert(raw_text.split('\n'));

	return genes;
}

void GeneSelectionDialog::phenotypesChanged()
{
	//update GUI
	QByteArrayList tmp;
	foreach(const Phenotype& pheno, phenotypes_)
	{
		tmp << pheno.name();
	}

	ui_->le_hpo->setText(tmp.join("; "));

	QString tooltip = QString("Phenotype/inheritance filter based on HPO terms.<br><br>Notes:<br>- This functionality is only available when NGSD is enabled.")
			+ "<br>- Filters based on the phenotype-associated gene loci including 5000 flanking bases.";
	if (!phenotypes_.isEmpty())
	{
		tooltip += "<br><br><nobr>Currently selected HPO terms:</nobr>";
		foreach(const Phenotype& pheno, phenotypes_)
		{
			tooltip += "<br><nobr>" + pheno.toString() + "</nobr>";
		}
	}
	ui_->le_hpo->setToolTip(tooltip);
}

void GeneSelectionDialog::editPhenotypes()
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

void GeneSelectionDialog::setPhenotypes(const PhenotypeList& phenotypes)
{
	phenotypes_ = phenotypes;
	phenotypesChanged();
}

void GeneSelectionDialog::importHPO()
{
	if(variant_filter_widget_!=nullptr)
	{
		phenotypes_ = variant_filter_widget_->phenotypes();
		phenotypesChanged();
	}
}

void GeneSelectionDialog::importROI()
{
	if(variant_filter_widget_!=nullptr)
	{
		ui_->cb_target_region->setCurrentText(variant_filter_widget_->targetRegionDisplayName());
	}
}

void GeneSelectionDialog::determineGenes()
{
	try
	{
		QApplication::setOverrideCursor(Qt::BusyCursor);
		NGSD db;
		GeneSet genes;

		//get genes from phenotype filter
		if (phenotypes_.count() > 0)
		{
			foreach (const Phenotype& phenotype, phenotypes_)
			{
				genes << db.phenotypeToGenes(db.phenotypeIdByAccession(phenotype.accession()), false);
			}
		}

		//get genes from target region
		QString selected_target_region = ui_->cb_target_region->currentText();
		if (!((selected_target_region == "") || (selected_target_region == "none")))
		{
			TargetRegionInfo target_region_info;
			FilterWidget::loadTargetRegionData(target_region_info, selected_target_region);
			if (target_region_info.genes.count() > 0)
			{
				if(genes.count() > 0)
				{
					genes = genes.intersect(target_region_info.genes);
				}
				else
				{
					genes = target_region_info.genes;
				}
			}
		}

		// set genes:
		ui_->te_genes->setPlainText(genes.toStringList().join('\n'));

		QApplication::restoreOverrideCursor();
	}
	catch (Exception& e)
	{
		GUIHelper::showException(this, e, "Error determine genes.");
	}
}

void GeneSelectionDialog::loadTargetRegions()
{
	ui_->cb_target_region->blockSignals(true);


	ui_->cb_target_region->clear();
	ui_->cb_target_region->addItem("", "");
	ui_->cb_target_region->addItem("none", "");
	ui_->cb_target_region->insertSeparator(ui_->cb_target_region->count());

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

			ui_->cb_target_region->addItem("Processing system: " + name, "Processing system: " + name);
		}
		ui_->cb_target_region->insertSeparator(ui_->cb_target_region->count());

		//load ROIs of NGSD sub-panels
		foreach(const QString& subpanel, db.subPanelList(false))
		{
			ui_->cb_target_region->addItem("Sub-panel: " + subpanel, "Sub-panel: " + subpanel);
		}
		ui_->cb_target_region->insertSeparator(ui_->cb_target_region->count());
	}

	//load additional ROIs from settings
	QStringList rois = Settings::stringList("target_regions", true);
	std::sort(rois.begin(), rois.end(), [](const QString& a, const QString& b){return QFileInfo(a).fileName().toUpper() < QFileInfo(b).fileName().toUpper();});
	foreach(const QString& roi_file, rois)
	{
		QFileInfo info(roi_file);
		ui_->cb_target_region->addItem(info.fileName(), roi_file);
	}

	ui_->cb_target_region->blockSignals(false);

}
