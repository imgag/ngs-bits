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

	//Returns the selected phenotypes
	const PhenotypeList& selectedPhenotypes() const;

signals:
	void phenotypeSelectionChanged();

private slots:
	void addPhenotypeToSelection(QString name);
	void deletePhenotype(QListWidgetItem* item);
	void updateSelectedPhenotypeList();
	void removeByContextMenu();
	void addParentsByContextMenu();

private:
	Ui::PhenotypeSelectionWidget ui_;
	PhenotypeList phenos_;
};

#endif // PHENOTYPESELECTIONWIDGET_H
