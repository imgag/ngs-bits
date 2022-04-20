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
	selected_sources_ = sources;

	foreach (QObject* o, ui->sourceBox->children())
	{
		QCheckBox* box = qobject_cast<QCheckBox*>(o);
		if (box == nullptr) continue;

		if (sources.contains(PhenotypeSource::sourceFromString(box->objectName())))
		{
			box->setChecked(true);
		}
	}
}

void PhenotypeSourceEvidenceSelector::setEvidences(QList<PhenotypeEvidence::Evidence> evidences)
{
	selected_evidences_ = evidences;

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
	return selected_sources_;
}

QList<PhenotypeEvidence::Evidence> PhenotypeSourceEvidenceSelector::selectedEvidences()
{
	return selected_evidences_;
}


void PhenotypeSourceEvidenceSelector::updateEvidenceSelection()
{
	QList <PhenotypeEvidence::Evidence> new_selected;

	foreach (QObject* o, ui->evidenceBox->children())
	{
		QCheckBox* box = qobject_cast<QCheckBox*>(o);
		if (box == nullptr) continue;

		if (box->isChecked())
		{
			new_selected.append(PhenotypeEvidence::evidenceFromString(box->objectName()));
		}
	}
	selected_evidences_ = new_selected;
}

void PhenotypeSourceEvidenceSelector::updateSourceSelection()
{
	QList <PhenotypeSource::Source> new_selected;

	foreach (QObject* o, ui->sourceBox->children())
	{
		QCheckBox* box = qobject_cast<QCheckBox*>(o);
		if (box == nullptr) continue;

		if (box->isChecked())
		{
			new_selected.append(PhenotypeSource::sourceFromString(box->objectName()));
		}
	}
	selected_sources_ = new_selected;
}
