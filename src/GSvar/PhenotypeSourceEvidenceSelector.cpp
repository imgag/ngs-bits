#include "PhenotypeSourceEvidenceSelector.h"
#include "ui_PhenotypeSourceEvidenceSelector.h"
#include <QMessageBox>


PhenotypeSourceEvidenceSelector::PhenotypeSourceEvidenceSelector(QWidget *parent) :
	QWidget(parent),
	ui(new Ui::PhenotypeSourceEvidenceSelector)
{
	ui->setupUi(this);

	connect(ui->CLINVAR, SIGNAL(toggled(bool)), SLOT(updateSourceSelection()));
	connect(ui->HPO, SIGNAL(toggled(bool)), SLOT(updateSourceSelection()));
	connect(ui->DECIPHER, SIGNAL(toggled(bool)), SLOT(updateSourceSelection()));
	connect(ui->GENCC, SIGNAL(toggled(bool)), SLOT(updateSourceSelection()));
	connect(ui->HGMD, SIGNAL(toggled(bool)), SLOT(updateSourceSelection()));
	connect(ui->OMIM, SIGNAL(toggled(bool)), SLOT(updateSourceSelection()));

	connect(ui->NA, SIGNAL(toggled(bool)), SLOT(updateEvidenceSelection()));
	connect(ui->LOW, SIGNAL(toggled(bool)), SLOT(updateEvidenceSelection()));
	connect(ui->MED, SIGNAL(toggled(bool)), SLOT(updateEvidenceSelection()));
	connect(ui->HIGH, SIGNAL(toggled(bool)), SLOT(updateEvidenceSelection()));
}

PhenotypeSourceEvidenceSelector::~PhenotypeSourceEvidenceSelector()
{
	delete ui;
}

void PhenotypeSourceEvidenceSelector::setSources(QList<PhenotypeSource::Source> sources)
{
	selectedSources_ = sources;

	foreach (QObject* o, ui->sourceBox->children())
	{
		QCheckBox* box = qobject_cast<QCheckBox*>(o);
		if (box == nullptr) continue;

		if (sources.contains(PhenotypeSource::SourceFromString(box->objectName())))
		{
			box->setChecked(true);
		}
	}
}

void PhenotypeSourceEvidenceSelector::setEvidences(QList<PhenotypeEvidence::Evidence> evidences)
{
	selectedEvidences_ = evidences;

	foreach (QObject* o, ui->evidenceBox->children())
	{
		QCheckBox* box = qobject_cast<QCheckBox*>(o);
		if (box == nullptr) continue;

		if (evidences.contains(PhenotypeEvidence::evidenceFromString(box->objectName())))
		{
			box->setChecked(true);
		}
	}
}

QList<PhenotypeSource::Source> PhenotypeSourceEvidenceSelector::selectedSources()
{
	return selectedSources_;
}

QList<PhenotypeEvidence::Evidence> PhenotypeSourceEvidenceSelector::selectedEvidences()
{
	return selectedEvidences_;
}


void PhenotypeSourceEvidenceSelector::updateEvidenceSelection()
{
	QList <PhenotypeEvidence::Evidence> newSelected;

	foreach (QObject* o, ui->evidenceBox->children())
	{
		QCheckBox* box = qobject_cast<QCheckBox*>(o);
		if (box == nullptr) continue;

		if (box->isChecked())
		{
			newSelected.append(PhenotypeEvidence::evidenceFromString(box->objectName()));
		}
	}
	selectedEvidences_ = newSelected;
}

void PhenotypeSourceEvidenceSelector::updateSourceSelection()
{
	QList <PhenotypeSource::Source> newSelected;

	foreach (QObject* o, ui->sourceBox->children())
	{
		QCheckBox* box = qobject_cast<QCheckBox*>(o);
		if (box == nullptr) continue;

		if (box->isChecked())
		{
			newSelected.append(PhenotypeSource::SourceFromString(box->objectName()));
		}
	}
	selectedSources_ = newSelected;
}
