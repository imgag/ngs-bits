#ifndef PHENOTYPESELECTOR_H
#define PHENOTYPESELECTOR_H

#include <QWidget>
#include <QListWidgetItem>
#include "NGSD.h"

namespace Ui {
class PhenotypeSelector;
}

///Widget to select HPO phenotypes.
class PhenotypeSelector
	: public QWidget
{
	Q_OBJECT

public:
	explicit PhenotypeSelector(QWidget *parent = 0);
	~PhenotypeSelector();

	///Initializes the widget with all phenotypes
	void init();

	///Returns HTML-formatted details for the currently selected item.
	QString selectedItemDetails(bool show_name, bool show_genes);

signals:
	///Signal that a new item was clicked (mainly to update details).
	QString phenotypeChanged(QString);
	///Signal that a new item was double-clicked (for selection).
	QString phenotypeActivated(QString);

private slots:
	void search(QString text);
	void itemChanged(QListWidgetItem* item);
	void itemActivated(QListWidgetItem* item);

private:
	Ui::PhenotypeSelector *ui;
	NGSD db_;
};

#endif // PHENOTYPESELECTOR_H
