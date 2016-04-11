#ifndef PHENOTYPESELECTOR_H
#define PHENOTYPESELECTOR_H

#include <QWidget>
#include <QListWidgetItem>
#include "NGSD.h"

namespace Ui {
class PhenotypeSelector;
}

class PhenotypeSelector
	: public QWidget
{
	Q_OBJECT

public:
	explicit PhenotypeSelector(QWidget *parent = 0);
	~PhenotypeSelector();

signals:
	QString phenotypeSelected(QString);

private slots:
	void search(QString text);
	void itemDoubleClicked(QListWidgetItem* item);
	void listContextMenu(QPoint p);

private:
	Ui::PhenotypeSelector *ui;
	NGSD db_;
};

#endif // PHENOTYPESELECTOR_H
