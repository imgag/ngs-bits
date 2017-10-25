#include "PhenotypeSelectionWidget.h"

PhenotypeSelectionWidget::PhenotypeSelectionWidget(QWidget *parent)
	: QWidget(parent)
	, ui_()
	, phenos_()
{
	ui_.setupUi(this);

	ui_.pheno_sel->setDetailsWidget(ui_.pheno_details);
	connect(ui_.pheno_sel, SIGNAL(phenotypeActivated(QString)), this, SLOT(copyPhenotype()));
	connect(ui_.pheno_list, SIGNAL(itemDoubleClicked(QListWidgetItem*)), this, SLOT(deletePhenotype(QListWidgetItem*)));
}

void PhenotypeSelectionWidget::setPhenotypes(const QList<Phenotype>& phenos)
{
	phenos_ = phenos;

	updateSelectedPhenotypeList();
}

void PhenotypeSelectionWidget::copyPhenotype()
{
	const Phenotype& phenotype = ui_.pheno_sel->selectedPhenotype();
	if (!phenos_.contains(phenotype))
	{
		phenos_.append(phenotype);
	}

	updateSelectedPhenotypeList();
}

void PhenotypeSelectionWidget::deletePhenotype(QListWidgetItem* item)
{
	int index = ui_.pheno_list->row(item);
	phenos_.removeAt(index);

	updateSelectedPhenotypeList();
}

void PhenotypeSelectionWidget::updateSelectedPhenotypeList()
{
	ui_.pheno_list->clear();
	foreach(const Phenotype& pheno, phenos_)
	{
		ui_.pheno_list->addItem(pheno.accession() + " - " + pheno.name());
	}

	//emit change signal
	emit phenotypeSelectionChanged();
}

const QList<Phenotype>& PhenotypeSelectionWidget::selectedPhenotypes() const
{
	return phenos_;
}
