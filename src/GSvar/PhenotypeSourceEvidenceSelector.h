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

	void setEvidences(QList<PhenotypeEvidence::Evidence> evidences);

	void setSources(QList<PhenotypeSource::Source> sources);

	QList<PhenotypeSource::Source> selectedSources();

	QList<PhenotypeEvidence::Evidence> selectedEvidences();

signals:

	void evidenceSelectionChanged();
	void sourceSelectionChanged();


public slots:
	void updateSourceSelection();
	void updateEvidenceSelection();
private:

	QList<PhenotypeSource::Source> selected_sources_;
	QList<PhenotypeEvidence::Evidence> selected_evidences_;

	QList<QCheckBox> source_boxes_;
	QList<QCheckBox> evidence_boxes_;

	Ui::PhenotypeSourceEvidenceSelector *ui;
};

#endif // PHENOTYPESOURCEEVIDENCESELECTOR_H
