#include "ExpressionOverviewWidget.h"
#include "FilterWidget.h"
#include "ui_ExpressionOverviewWidget.h"
#include "GlobalServiceProvider.h"
#include "LoginManager.h"
#include "NGSD.h"
#include "GUIHelper.h"
#include "PhenotypeSelectionWidget.h"
#include <QCheckBox>
#include <QMessageBox>

struct ExpressioData
{
	double tpm_mean;
	double tpm_stdev;
	double tpm_01_perc;
	double tpm_1_perc;
};

ExpressionOverviewWidget::ExpressionOverviewWidget(FilterWidget* variant_filter_widget, QWidget *parent) :
	QWidget(parent),
	ui_(new Ui::ExpressionOverviewWidget),
	variant_filter_widget_(variant_filter_widget)
{
	// skipp if no NGSD is available
	if (!LoginManager::active())
	{
		QMessageBox::warning(this, "Expression Level Widget", "Widget requires NGSD access!");
		return;
	}

	ui_->setupUi(this);

	initPhenotype();
	initTargetRegions();
	initProcessingSystems();
	initTissue();

	connect(ui_->le_hpo, SIGNAL(clicked(QPoint)), this, SLOT(editPhenotypes()));
	connect(ui_->b_import_genes, SIGNAL(clicked(bool)), this, SLOT(applyGeneFilter()));
	connect(ui_->b_apply, SIGNAL(clicked(bool)), this, SLOT(showExpressionData()));

	if(variant_filter_widget_ != nullptr)
	{
		ui_->b_import_phenotype->setEnabled(true);
		ui_->b_import_target_region->setEnabled(true);
		connect(ui_->b_import_phenotype, SIGNAL(clicked(bool)), this, SLOT(importHPO()));
		connect(ui_->b_import_target_region, SIGNAL(clicked(bool)), this, SLOT(importROI()));
	}


}

ExpressionOverviewWidget::~ExpressionOverviewWidget()
{
	delete ui_;
}

void ExpressionOverviewWidget::initPhenotype()
{

}

void ExpressionOverviewWidget::initTargetRegions()
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

void ExpressionOverviewWidget::initProcessingSystems()
{
	NGSD db;

	//get processing systems
	QList<int> sys_ids = db.getValuesInt("SELECT `id` FROM `processing_system` WHERE `type`='RNA' ORDER BY `name_manufacturer`");

	ui_->cb_processing_system->clear();
	foreach (int id, sys_ids)
	{
		ProcessingSystemData sys_data = db.getProcessingSystemData(id);
		ui_->cb_processing_system->addItem(sys_data.name);
	}
}

void ExpressionOverviewWidget::initTissue()
{
	QStringList tissues = NGSD().getEnum("sample", "tissue");
	QVBoxLayout* vbox = new QVBoxLayout;
	foreach (const QString& tissue, tissues)
	{
		QCheckBox* cb_tissue = new QCheckBox(tissue);
		cb_tissue->setChecked(true);
		cb_tissue->setMinimumHeight(20);
		vbox->addWidget(cb_tissue);
	}
	ui_->sa_tissue->setLayout(vbox);
}

void ExpressionOverviewWidget::phenotypesChanged()
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

void ExpressionOverviewWidget::editPhenotypes()
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

void ExpressionOverviewWidget::importHPO()
{
	if(variant_filter_widget_!=nullptr)
	{
		phenotypes_ = variant_filter_widget_->phenotypes();
		phenotypesChanged();
	}
}

void ExpressionOverviewWidget::importROI()
{
	if(variant_filter_widget_!=nullptr)
	{
		ui_->cb_target_region->setCurrentText(variant_filter_widget_->targetRegionDisplayName());
	}
}

GeneSet ExpressionOverviewWidget::getGeneSet()
{
	GeneSet genes;
	try
	{
		// parse text field
		QByteArray raw_text = ui_->te_selected_genes->toPlainText().replace(" ", "\n").replace("\t", "\n").replace(",", "\n").replace(";", "\n").replace("\r", "").toUtf8();
		genes.insert(raw_text.split('\n'));
	}
	catch (Exception& e)
	{
		GUIHelper::showException(this, e, "Error opening RNA expression file.");
	}
	return genes;
}

void ExpressionOverviewWidget::showExpressionData()
{
	try
	{
		QApplication::setOverrideCursor(Qt::BusyCursor);
		NGSD db;

		// debug
		QTime timer;
		timer.start();

		//get GeneSet
		GeneSet gene_set = getGeneSet();

		//skip if no genes are selected
		if(gene_set.count() < 1) WARNING(ArgumentException, "No genes selected for look-up. Please select at least one gene.");

		//get processing system
		int sys_id = db.processingSystemId(ui_->cb_processing_system->currentText());
		ProcessingSystemData sys_info = db.getProcessingSystemData(sys_id);


		//get tissue
		QSet<QString> selected_tissues;
		foreach (QCheckBox* cb_tissue, ui_->sa_tissue->findChildren<QCheckBox*>())
		{
			if (cb_tissue->isChecked())
			{
				selected_tissues.insert(cb_tissue->text());
			}
		}

		//get processed sample IDs for each processing system and tissue
		QMap<QPair<QString, QString>, ExpressioData> expression;
		foreach (const QString& tissue, selected_tissues)
		{
			QSet<int> current_cohort = db.getRNACohort(sys_id, tissue);

			// skip columns with no samples
			if(current_cohort.size() == 0) continue;

			foreach (const QByteArray& gene, gene_set)
			{
				//get expression data for each gene
				QVector<double> tpm_values = db.getGeneExpressionValues(gene, current_cohort);

				if (tpm_values.size() > 0)
				{
					ExpressioData data;
					data.tpm_mean = BasicStatistics::mean(tpm_values);
					data.tpm_stdev = BasicStatistics::stdev(tpm_values, data.tpm_mean);

					int n_tpm01=0, n_tpm1=0;
					foreach (double tpm, tpm_values)
					{
						if(tpm >= 0.1)
						{
							n_tpm01++;
							if(tpm >= 1.0)
							{
								n_tpm1++;
							}
						}
					}

					data.tpm_01_perc = (double) n_tpm01 / tpm_values.size();
					data.tpm_1_perc = (double) n_tpm1 / tpm_values.size();

					expression.insert(QPair<QString, QString>(tissue, gene), data);
				}

			}
		}



		// init table
		ui_->tw_expression->setSortingEnabled(false);
		ui_->tw_expression->setColumnCount(1 + (4 * selected_tissues.count()));
		ui_->tw_expression->setRowCount(gene_set.count());

		int col_idx = 0;

		ui_->tw_expression->setHorizontalHeaderItem(col_idx++, new QTableWidgetItem("\n\ngene"));
		//iterate over all processing systems with data
		foreach (auto tissue, selected_tissues)
		{
			ui_->tw_expression->setHorizontalHeaderItem(col_idx++, new QTableWidgetItem(sys_info.name + "\nTissue: " + tissue + "\nmean"));
			ui_->tw_expression->setHorizontalHeaderItem(col_idx++, new QTableWidgetItem("\n\nstdev"));
			ui_->tw_expression->setHorizontalHeaderItem(col_idx++, new QTableWidgetItem("\n\n(tpm > 0.1) %"));
			ui_->tw_expression->setHorizontalHeaderItem(col_idx++, new QTableWidgetItem("\n\n(tpm > 1) %"));

		}

		// fill table
		int row_idx = 0;
		foreach (const QString& gene, gene_set)
		{
			col_idx = 0;
			ui_->tw_expression->setItem(row_idx, col_idx++, new QTableWidgetItem(gene));

			foreach (auto tissue, selected_tissues)
			{
				if(expression.contains(QPair<QString, QString>(tissue, gene)))
				{
					const ExpressioData& data = expression.value(QPair<QString, QString>(tissue, gene));
					ui_->tw_expression->setItem(row_idx, col_idx++, GUIHelper::createTableItem(data.tpm_mean));
					ui_->tw_expression->setItem(row_idx, col_idx++, GUIHelper::createTableItem(data.tpm_stdev));
					ui_->tw_expression->setItem(row_idx, col_idx++, GUIHelper::createTableItem(data.tpm_01_perc * 100));
					ui_->tw_expression->setItem(row_idx, col_idx++, GUIHelper::createTableItem(data.tpm_1_perc * 100));
				}
				else
				{
					for (int i = 0; i < 4; ++i) ui_->tw_expression->setItem(row_idx, col_idx++, GUIHelper::createTableItem(""));
				}

			}
			row_idx++;
		}

		//hide vertical header
		ui_->tw_expression->verticalHeader()->setVisible(false);

		//enable sorting
		ui_->tw_expression->setSortingEnabled(true);

		GUIHelper::resizeTableCells(ui_->tw_expression);

		QApplication::restoreOverrideCursor();

		qDebug() << "Get processing system expression level: " << Helper::elapsedTime(timer);

	}
	catch (Exception& e)
	{
		GUIHelper::showException(this, e, "Error opening RNA expression file.");
	}
}

void ExpressionOverviewWidget::applyGeneFilter()
{
	try
	{
		QApplication::setOverrideCursor(Qt::BusyCursor);
		NGSD db;
		GeneSet genes;

		qDebug() << "determine genes";

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
		ui_->te_selected_genes->setPlainText(genes.toStringList().join('\n'));

		QApplication::restoreOverrideCursor();
	}
	catch (Exception& e)
	{
		GUIHelper::showException(this, e, "Error determine genes.");
	}
}
