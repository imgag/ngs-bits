#ifndef ANALYSISTIMEPLOT_H
#define ANALYSISTIMEPLOT_H

#include <QWidget>
#include "ui_AnalysisTimePlot.h"

class AnalysisTimePlot : public QWidget
{
	Q_OBJECT

public:
	AnalysisTimePlot(QWidget* parent);

protected slots:
	void updatePlot();
	void copyToClipboard();

private:
	Ui::AnalysisTimePlot ui_;
	QString getSqlQuery(QString add_fields = QString());
};

#endif // ANALYSISTIMEPLOT_H
