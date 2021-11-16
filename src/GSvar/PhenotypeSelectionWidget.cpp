#include "PhenotypeSelectionWidget.h"
#include <iostream>
#include <qobject.h>
#include <QMessageBox>

PhenotypeSelectionWidget::PhenotypeSelectionWidget(QWidget *parent)
	: QWidget(parent)
	, ui_()
	, phenos_()
{
	ui_.setupUi(this);

	ui_.pheno_sel->setDetailsWidget(ui_.pheno_details);
	connect(ui_.pheno_sel, SIGNAL(phenotypeActivated(QString)), this, SLOT(copyPhenotype(QString)));
	connect(ui_.pheno_list, SIGNAL(itemDoubleClicked(QListWidgetItem*)), this, SLOT(deletePhenotype(QListWidgetItem*)));
}

void PhenotypeSelectionWidget::setPhenotypes(const PhenotypeList& phenos)
{
	phenos_ = phenos;

	updateSelectedPhenotypeList();
}

void PhenotypeSelectionWidget::copyPhenotype(QString name)
{
	const Phenotype& phenotype = ui_.pheno_sel->nameToPhenotype(name.toLatin1());
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
	foreach(const Phenotype& pheno, phenos_)
	{
		ui_.pheno_list->addItem(pheno.accession() + " - " + pheno.name());
	}

	//emit change signal
	emit phenotypeSelectionChanged();
}

const PhenotypeList& PhenotypeSelectionWidget::selectedPhenotypes() const
{
	return phenos_;
}

QStringList PhenotypeSelectionWidget::getSelectedSources() {
	QStringList list;
	std::cout << "entered getSelectedSources()" << std::endl;
	foreach (QObject* o, ui_.databaseBox->children())
	{
		QRadioButton* button = qobject_cast<QRadioButton*>(o);
		if (button == nullptr) continue;
		if (button->isChecked()) {
			list.append((button->objectName()));
		}
	}
	return list;
}

QStringList PhenotypeSelectionWidget::getSelectedEvidences() {
	QStringList list;
	std::cout << "entered getSelectedEvidences()" << std::endl;
	for (int i=0; i < ui_.evidenceBox->children().length(); i++)
	{
		QObject* o = ui_.evidenceBox->children()[i];
		QRadioButton* button = qobject_cast<QRadioButton*>(o);

		if (button == nullptr) continue;
		if (button->isChecked()) {
			list.append((button->objectName()));
		}
	}
	return list;
}
