#ifndef COHORTANALYSISWIDGET_H
#define COHORTANALYSISWIDGET_H

#include <QWidget>
#include "ui_CohortAnalysisWidget.h"


class CohortAnalysisWidget
	: public QWidget
{
	Q_OBJECT

public:
	CohortAnalysisWidget(QWidget* parent = 0);

protected:
	QString baseQuery();

protected slots:
	void updateOutputTable();

private:
	Ui::CohortAnalysisWidget ui_;
};

#endif // COHORTANALYSISWIDGET_H
