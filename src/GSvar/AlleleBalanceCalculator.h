#ifndef ALLELEBALANCECALCULATOR_H
#define ALLELEBALANCECALCULATOR_H

#include <QWidget>
#include "ui_AlleleBalanceCalculator.h"

class AlleleBalanceCalculator
	: public QWidget
{
	Q_OBJECT

public:
	AlleleBalanceCalculator(QWidget* parent = 0);

private slots:
	void updateResult();

private:
	Ui::AlleleBalanceCalculator ui_;
};

#endif // ALLELEBALANCECALCULATOR_H
