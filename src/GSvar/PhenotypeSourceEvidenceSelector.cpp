#include "PhenotypeSourceEvidenceSelector.h"
#include "ui_PhenotypeSourceEvidenceSelector.h"

PhenotypeSourceEvidenceSelector::PhenotypeSourceEvidenceSelector(QWidget *parent) :
	QWidget(parent),
	ui(new Ui::PhenotypeSourceEvidenceSelector)
{
	ui->setupUi(this);

	// connect(ui_.cascade_widget, SIGNAL(filterCascadeChanged()), this, SLOT(updateFilterName()));
	connect(ui->CLINVAR, SIGNAL(toggled(bool)), SLOT(updateSourceSelection()));
	connect(ui->HPO, SIGNAL(toggled(bool)), SLOT(updateSourceSelection()));
	connect(ui->DECIPHER, SIGNAL(toggled(bool)), SLOT(updateSourceSelection()));
	connect(ui->GENCC, SIGNAL(toggled(bool)), SLOT(updateSourceSelection()));
	connect(ui->HGMC, SIGNAL(toggled(bool)), SLOT(updateSourceSelection()));
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

/*

foreach (QObject* o, ui_.databaseBox->children())
		{
				QCheckBox* button = qobject_cast<QCheckBox*>(o);
				if (button == nullptr) continue;

				if ( ! sources.contains(button->objectName()))
				{
						button->setChecked(false);
				}
		}

*/
void PhenotypeSourceEvidenceSelector::setSources(QList<PhenotypeSource> sources)
{
	selectedSources_ = sources;

	foreach (QObject* o, ui->sourceBox->children())
	{
		QCheckBox* box = qobject_cast<QCheckBox*>(o);
		if (box == nullptr) continue;

		if (sources.contains(SourceFromString(box->objectName())))
		{
			box->setChecked(true);
		}
	}
}

void PhenotypeSourceEvidenceSelector::setEvidences(QList<PhenotypeEvidence> evidences)
{
	selectedEvidences_ = evidences;

	foreach (QObject* o, ui->evidenceBox->children())
	{
		QCheckBox* box = qobject_cast<QCheckBox*>(o);
		if (box == nullptr) continue;

		if (evidences.contains(evidenceFromString(box->objectName())))
		{
			box->setChecked(true);
		}
	}
}

QList<PhenotypeSource> PhenotypeSourceEvidenceSelector::selectedSources()
{
	return selectedSources_;
}

QList<PhenotypeEvidence> PhenotypeSourceEvidenceSelector::selectedEvidences()
{
	return selectedEvidences_;
}


void PhenotypeSourceEvidenceSelector::updateEvidenceSelection()
{

}

void PhenotypeSourceEvidenceSelector::updateSourceSelection()
{

}
