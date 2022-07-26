#ifndef EXPRESSIONOVERVIEWWIDGET_H
#define EXPRESSIONOVERVIEWWIDGET_H

#include "FilterWidget.h"
#include "GeneSet.h"
#include <PhenotypeList.h>
#include <QWidget>

namespace Ui {
class ExpressionOverviewWidget;
}

class ExpressionOverviewWidget : public QWidget
{
	Q_OBJECT

public:
	explicit ExpressionOverviewWidget(FilterWidget* variant_filter_widget = nullptr, QWidget *parent = 0);
	~ExpressionOverviewWidget();

private:
	Ui::ExpressionOverviewWidget *ui_;
	FilterWidget* variant_filter_widget_;
	PhenotypeList phenotypes_;

	//init
	void initPhenotype();
	void initTargetRegions();
	void initProcessingSystems();
	void initTissue();

	GeneSet getGeneSet();

private slots:
	void showExpressionData();
	void applyGeneFilter();

	//phenotype
	void phenotypesChanged();
	void editPhenotypes();

	//import from variant list
	void importHPO();
	void importROI();
};

#endif // EXPRESSIONOVERVIEWLWIDGET_H
