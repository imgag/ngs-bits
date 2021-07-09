#ifndef PHENOTYPESELECTOR_H
#define PHENOTYPESELECTOR_H

#include <QWidget>
#include <QListWidgetItem>
#include <QTextEdit>
#include "NGSD.h"
#include "Phenotype.h"

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

	///Sets the associated widget which is autoamtically updated when a phenotype is selected.
	void setDetailsWidget(QTextEdit* edit);

	///Initializes the widget with all phenotypes
	void init();

	///Returns the selected phenotype
	const Phenotype& nameToPhenotype(QByteArray name) const;

	///React on enter key
	void keyPressEvent(QKeyEvent *event) override;

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
	///Returns HTML-formatted details for the currently selected item.
	QString selectedItemDetails(bool show_name, bool show_genes);

	Ui::PhenotypeSelector *ui;
	QTextEdit* details_;
	mutable NGSD db_;
};

#endif // PHENOTYPESELECTOR_H
