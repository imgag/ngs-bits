#ifndef PHENOTYPESOURCEEVIDENCESELECTOR_H
#define PHENOTYPESOURCEEVIDENCESELECTOR_H

#include <QWidget>
#include "Phenotype.h"
#include <QCheckBox>

namespace Ui {
class PhenotypeSourceEvidenceSelector;
}

class PhenotypeSourceEvidenceSelector : public QWidget
{
	Q_OBJECT

public:
	explicit PhenotypeSourceEvidenceSelector(QWidget *parent = 0);
	~PhenotypeSourceEvidenceSelector();

	void setEvidences(QList<PhenotypeEvidence> evidences);

	void setSources(QList<PhenotypeSource> sources);

	QList<PhenotypeSource> selectedSources();

	QList<PhenotypeEvidence> selectedEvidences();

signals:

	void evidenceSelectionChanged();
	void sourceSelectionChanged();


public slots:
	void updateSourceSelection();
	void updateEvidenceSelection();
private:

	QList<PhenotypeSource> selectedSources_;
	QList<PhenotypeEvidence> selectedEvidences_;

	QList<QCheckBox> sourceBoxes_;
	QList<QCheckBox> evidenceBoxes_;

	Ui::PhenotypeSourceEvidenceSelector *ui;
};

#endif // PHENOTYPESOURCEEVIDENCESELECTOR_H
