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
	void setPhenotypes(const QList<Phenotype>& phenos);

	//Returns the selected phenotypes
	const QList<Phenotype>& selectedPhenotypes() const;

signals:
	void phenotypeSelectionChanged();

private slots:
	void copyPhenotype(QString name);
	void deletePhenotype(QListWidgetItem* item);
	void updateSelectedPhenotypeList();

private:
	Ui::PhenotypeSelectionWidget ui_;
	QList<Phenotype> phenos_;
};

#endif // PHENOTYPESELECTIONWIDGET_H
