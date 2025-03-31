#include "PhenotypeSelectionWidget.h"
#include <iostream>
#include <qobject.h>
#include <QMessageBox>
#include <QAction>

PhenotypeSelectionWidget::PhenotypeSelectionWidget(QWidget *parent)
	: QWidget(parent)
	, ui_()
	, phenos_()
{
	ui_.setupUi(this);

	ui_.pheno_sel->setDetailsWidget(ui_.pheno_details);
	connect(ui_.pheno_sel, SIGNAL(phenotypeActivated(QString)), this, SLOT(addPhenotypeToSelection(QString)));
	connect(ui_.pheno_list, SIGNAL(itemDoubleClicked(QListWidgetItem*)), this, SLOT(deletePhenotype(QListWidgetItem*)));

	QAction* action = new QAction(QIcon(":/Icons/Remove.png"), "Remove");
	ui_.pheno_list->addAction(action);
	connect(action, SIGNAL(triggered(bool)), this, SLOT(removeByContextMenu()));

	action = new QAction(QIcon(":/Icons/Add.png"), "Add parent(s)");
	ui_.pheno_list->addAction(action);
	connect(action, SIGNAL(triggered(bool)), this, SLOT(addParentsByContextMenu()));
}

void PhenotypeSelectionWidget::setPhenotypes(const PhenotypeList& phenos)
{
	phenos_ = phenos;

	updateSelectedPhenotypeList();
}

void PhenotypeSelectionWidget::addPhenotypeToSelection(QString name)
{
	const Phenotype& phenotype = ui_.pheno_sel->nameToPhenotype(name.toUtf8());
	if (!phenos_.containsAccession(phenotype.accession()))
	{
		phenos_ << phenotype;
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
    for (const Phenotype& pheno : phenos_)
	{
		ui_.pheno_list->addItem(pheno.accession() + " - " + pheno.name());
	}

	//emit change signal
	emit phenotypeSelectionChanged();
}

void PhenotypeSelectionWidget::removeByContextMenu()
{
	QList<QListWidgetItem*> items = ui_.pheno_list->selectedItems();
	if (items.count()!=1) return;

	emit deletePhenotype(items[0]);
}

void PhenotypeSelectionWidget::addParentsByContextMenu()
{
	QList<QListWidgetItem*> items = ui_.pheno_list->selectedItems();
	if (items.count()!=1) return;

	//determine row index
	int index = ui_.pheno_list->row(items[0]);

	//determine parent
	NGSD db;
	int pheno_id = db.phenotypeIdByAccession(phenos_[index].accession());
	PhenotypeList parents = db.phenotypeParentTerms(pheno_id, false);

	//add parents to selection
	foreach(const Phenotype& parent, parents)
	{
		phenos_ << parent;
	}
	updateSelectedPhenotypeList();

}

const PhenotypeList& PhenotypeSelectionWidget::selectedPhenotypes() const
{
	return phenos_;
}
