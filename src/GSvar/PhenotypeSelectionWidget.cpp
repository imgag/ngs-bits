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

void PhenotypeSelectionWidget::setSources(const QStringList sources)
{
	if (sources.length() == 0) return; // leaves all checked as is baseline

	foreach (QObject* o, ui_.databaseBox->children())
	{
		QCheckBox* button = qobject_cast<QCheckBox*>(o);
		if (button == nullptr) continue;

		if ( ! sources.contains(button->objectName()))
		{
			button->setChecked(false);
		}
	}
}

void PhenotypeSelectionWidget::setEvidences(const QStringList evidences)
{
	if (evidences.length() == 0) return; // leaves all checked as is baseline

	if ( ! evidences.contains("NA"))
	{
		ui_.NA->setChecked(false);
	}

	foreach (QObject* o, ui_.evidenceBox->children())
	{
		QRadioButton* button = qobject_cast<QRadioButton*>(o);
		if (button == nullptr) continue;
		if (evidences.contains(button->objectName()))
		{
			button->setChecked(true);
		}

	}
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

QStringList PhenotypeSelectionWidget::selectedSources() {
	QStringList list;
	foreach (QObject* o, ui_.databaseBox->children())
	{
		QCheckBox* button = qobject_cast<QCheckBox*>(o);
		if (button == nullptr) continue;
		if (button->isChecked()) {
			list.append((button->objectName()));
		}
	}
	return list;
}

QStringList PhenotypeSelectionWidget::selectedEvidences() {
	QStringList list;

	if (ui_.NA->isChecked()) list.append(ui_.NA->objectName());

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
