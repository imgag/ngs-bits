#ifndef EXPRESSIONLEVELWIDGET_H
#define EXPRESSIONLEVELWIDGET_H

#include "GeneSet.h"
#include <QWidget>

namespace Ui {
class ExpressionLevelWidget;
}

class ExpressionLevelWidget : public QWidget
{
	Q_OBJECT

public:
	explicit ExpressionLevelWidget(const GeneSet& genes, QWidget *parent = 0);
	~ExpressionLevelWidget();

private:
	Ui::ExpressionLevelWidget *ui_;
	GeneSet genes_;

	void loadData();
};

#endif // EXPRESSIONLEVELWIDGET_H
