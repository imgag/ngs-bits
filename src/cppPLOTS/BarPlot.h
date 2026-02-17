#ifndef BARPLOT_H
#define BARPLOT_H

#include "cppPLOTS_global.h"
#include <QString>
#include <QHash>
#include <QtCharts/QChartView>
#include <QtCharts/QBarSeries>
#include <QtCharts/QBarSet>
#include <QtCharts/QBarCategoryAxis>
#include <QtCharts/QValueAxis>
#include <QtCharts/QLegend>

// Creates a bar plot PNG image
class CPPPLOTSSHARED_EXPORT BarPlot
{
public:
	BarPlot();

	void setValues(const QList<int>& values, const QList<QString>& labels, const QList<QString>& colors = QList<QString>());
	void setValues(const QList<double>& values, const QList<QString>& labels, const QList<QString>& colors = QList<QString>());
	void setXLabel(const QString& x_label);
	void setYLabel(const QString& y_label);
	void setXRange(double min, double max);
	void setYRange(double min, double max);
	void setLegendVisible(const bool& visible);
	void store(QString filename);

protected:
	QList<double> bars_;
	QList<QString> labels_;
	QList<QString> colors_;	
	bool is_legend_visible_;
	QString xlabel_;
	QString ylabel_;
	double ymin_;
	double ymax_;
	double xmin_;
	double xmax_;	
};

#endif
