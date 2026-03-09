#include "BarPlot.h"
#include "Exceptions.h"
#include "Helper.h"
#include "Log.h"
#include "BasicStatistics.h"
#include <QStringList>
#include <limits>
#include <QStandardPaths>
#include "Settings.h"

#include <QApplication>

BarPlot::BarPlot()
{
}

void BarPlot::setValues(const QList<int>& values, const QList<QString>& labels, const QList<QString>& colors)
{
	for(int i=0;i<values.count();++i)
	{
		bars_.append(double(values[i]));
		labels_.append(labels[i]);
		colors_.append(colors);
	}
}


void BarPlot::setValues(const QList<double>& values, const QList<QString>& labels, const QList<QString>& colors)
{
	for(int i=0;i<values.count();++i)
	{
		bars_.append(values[i]);
		labels_.append(labels[i]);
		colors_.append(colors);
	}
}


void BarPlot::addColorLegend(QString color, QString desc)
{
	color_legend_.insert(color, desc);
}

void BarPlot::store(QString filename)
{
	if (bars_.isEmpty()) return;

	// the code needs an instance of GUI app to work, we make sure it will work even without one
	QCoreApplication* app = QCoreApplication::instance();

	static int argc = 1;
	static char arg0[] = "test";
	static char* argv[] = { arg0, nullptr };

	if (!qobject_cast<QApplication*>(app))
	{
		qputenv("QT_QPA_PLATFORM", "offscreen");
		new QApplication(argc, argv);
	}
	QChart* chart = new QChart();
	QBarSeries* series = new QBarSeries();

	for (int i = 0; i < bars_.size(); ++i)
	{
		QBarSet* set = new QBarSet("");
		set->setColor(matplotlibColorToQColor(colors_[i]));

		for (int j = 0; j < bars_.size(); ++j)
		{
			if (j == i)
				*set << bars_[i];
			else
				*set << 0.0;
		}

		series->append(set);
	}

	chart->addSeries(series);
	chart->legend()->hide();

	// X axis categories
	QBarCategoryAxis* axisX = new QBarCategoryAxis();
	QStringList categories;
	if (!labels_.isEmpty() && labels_.size() == bars_.size())
		categories = labels_;
	else
		for (int i = 0; i < bars_.size(); ++i)
			categories << QString::number(i);

	axisX->append(categories);
	if (!xlabel_.isEmpty()) axisX->setTitleText(xlabel_);
	axisX->setLabelsAngle(-90);
	chart->addAxis(axisX, Qt::AlignBottom);
	series->attachAxis(axisX);

	// Y axis
	QValueAxis* axisY = new QValueAxis();
	if (!ylabel_.isEmpty()) axisY->setTitleText(ylabel_);

	if (BasicStatistics::isValidFloat(ymin_) &&
		BasicStatistics::isValidFloat(ymax_))
	{
		axisY->setRange(ymin_, ymax_);
	}

	chart->addAxis(axisY, Qt::AlignLeft);
	series->attachAxis(axisY);


	axisX->setGridLineVisible(false);
	axisY->setGridLineVisible(false);

	// render
	QChartView chartView(chart);
	chartView.resize(1000, 400); // 10x4 inches @ 100 dpi

	chartView.setRenderHint(QPainter::Antialiasing, true);
	chartView.setRenderHint(QPainter::TextAntialiasing, true);
	chartView.setRenderHint(QPainter::SmoothPixmapTransform, true);

	// chartView.show();
	QApplication::processEvents();

	QPixmap pixmap = chartView.grab();

	if (!pixmap.save(filename.replace("\\", "/"), "PNG"))
		THROW(ProgrammingException, "Could not save bar plot to file: " + filename);

	//delete chart;
}

QColor BarPlot::matplotlibColorToQColor(const QString &c)
{
	Log::error("Color = " + c);
	if (c == "b") return QColor(0, 0, 255);
	if (c == "k") return QColor(0, 0, 0);
	if (c == "r") return QColor(255, 0, 0);
	if (c == "g") return QColor(0, 128, 0);   // matplotlib green
	if (c == "c") return QColor(0, 255, 255);
	if (c == "y") return QColor(255, 255, 0);

	// fallback: try Qt parsing (for hex colors etc.)
	QColor qc(c);
	if (qc.isValid()) return qc;

	// last resort
	Log::warn("Unknown color '" + c + "' — using black.");
	return Qt::black;
}

