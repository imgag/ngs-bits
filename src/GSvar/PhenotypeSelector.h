#ifndef PHENOTYPESELECTOR_H
#define PHENOTYPESELECTOR_H

#include <QWidget>
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

private slots:
	void search(QString text);

private:
	Ui::PhenotypeSelector *ui;
	NGSD db_;
};

#endif // PHENOTYPESELECTOR_H
