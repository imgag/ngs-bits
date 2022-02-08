#ifndef COHORTEXPRESSIONDATAWIDGET_H
#define COHORTEXPRESSIONDATAWIDGET_H

#include <QWidget>

namespace Ui {
class CohortExpressionDataWidget;
}

class CohortExpressionDataWidget : public QWidget
{
	Q_OBJECT

public:
	explicit CohortExpressionDataWidget(QString tsv_filename, QWidget *parent = 0, QString project_name="", QString processing_system_name="");
	~CohortExpressionDataWidget();

private:
	void loadExpressionData();
	QString tsv_filename_;
	Ui::CohortExpressionDataWidget *ui_;
};

#endif // COHORTEXPRESSIONDATAWIDGET_H
