#ifndef SCATTERPLOT_H
#define SCATTERPLOT_H

#include "cppPLOTS_global.h"
#include <QString>
#include <QList>
#include <QHash>
#include <QtCharts/QChart>
#include <QtCharts/QScatterSeries>
#include <QtCharts/QLineSeries>
#include <QtCharts/QValueAxis>
#include <QtCharts/QLogValueAxis>
#include <QtCharts/QChartView>

// Creates a scatter plot PNG image
class CPPPLOTSSHARED_EXPORT ScatterPlot
{
public:
	ScatterPlot();
	void setValues(const QList< std::pair<double,double> >& values, const QList<QString>& colors = QList<QString>());
	void convertChromToX();
	void setXLabel(QString xlabel);
	void setXRange(double xmin, double xmax);
	void setYLabel(QString ylabel);
	void setYRange(double ymin, double ymax);
	void noXTicks()
	{
		noxticks_ = true;
	}
	void setYLogScale(bool yscale_log)
	{
		yscale_log_ = yscale_log;
	}
	void addVLine(double x);
	void addColorLegend(QString color,QString desc);
	void store(QString filename);

protected:
	//variables to store the plot data
	QList<std::pair<double,double>> points_;
	QList<double> vlines_;
	QList<QString> colors_;
	QHash<QString,QString> color_legend_;
	QString xlabel_;
	QString ylabel_;
	double ymin_;
	double ymax_;
	double xmin_;
	double xmax_;

	//variables to store the meta data
	bool yrange_set_;
	bool xrange_set_;
	bool yscale_log_;
	bool noxticks_;
};

#endif // SCATTERPLOT_H
