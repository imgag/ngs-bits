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

private:
	Ui::AnalysisTimePlot ui_;
};

#endif // ANALYSISTIMEPLOT_H
