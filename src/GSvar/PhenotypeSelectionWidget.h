#ifndef PHENOTYPESELECTIONWIDGET_H
#define PHENOTYPESELECTIONWIDGET_H

#include <QWidget>
#include <QListWidgetItem>
#include "ui_PhenotypeSelectionWidget.h"

class PhenotypeSelectionWidget
	: public QWidget
{
	Q_OBJECT

public:
	PhenotypeSelectionWidget(QWidget *parent = 0);

	//Sets an initial list of phenotypes
	void setPhenotypes(const PhenotypeList& phenos);
	//Sets which sources are initially checked (expects a Stringlist with the object names that should be checked or empty for all checked)
	void setSources(const QStringList sources);
	//Sets which evidences are initially checked (expects a Stringlist with the object names that should be checked or empty for all checked)
	void setEvidences(const QStringList evidences);

	//Returns the selected phenotypes
	const PhenotypeList& selectedPhenotypes() const;
	//Returns a list of of selected source databases
	QStringList selectedSources();
	//Returns a list of selected evidence/confidence levels
	QStringList selectedEvidences();

signals:
	void phenotypeSelectionChanged();

private slots:
	void copyPhenotype(QString name);
	void deletePhenotype(QListWidgetItem* item);
	void updateSelectedPhenotypeList();

private:
	Ui::PhenotypeSelectionWidget ui_;
	PhenotypeList phenos_;
};

#endif // PHENOTYPESELECTIONWIDGET_H
