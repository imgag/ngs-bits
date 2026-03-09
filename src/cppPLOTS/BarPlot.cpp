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

void BarPlot::setXLabel(const QString &x_label)
{
	xlabel_ = x_label;
}

void BarPlot::setYLabel(const QString &y_label)
{
	ylabel_ = y_label;
}

void BarPlot::setXRange(double min, double max)
{
	xmin_ = min;
	xmax_ = max;
}

void BarPlot::setYRange(double min, double max)
{
	ymin_ = min;
	ymax_ = max;
}

void BarPlot::setLegendVisible(const bool &visible)
{
	is_legend_visible_ = visible;
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
		QBarSet* set = new QBarSet(labels_[i]);
		set->setColor(QColor::fromString(colors_[i]));

		if (is_legend_visible_)
		{
			*set << bars_[i];
		}
		else
		{
			for (int j = 0; j < bars_.size(); ++j)
			{
				if (j == i)
					*set << bars_[i];
				else
					*set << 0.0;
			}
		}

		series->append(set);
	}

	chart->addSeries(series);

	// X axis categories
	QBarCategoryAxis* axisX = new QBarCategoryAxis();
	QStringList categories;
	if (!labels_.isEmpty() && labels_.size() == bars_.size())
		categories = labels_;
	else
		for (int i = 0; i < bars_.size(); ++i)
			categories << QString::number(i);

	// legend
	chart->legend()->setVisible(is_legend_visible_);
	chart->legend()->setAlignment(Qt::AlignRight);

	if (is_legend_visible_)
	{
		axisX->append(QStringList(""));
	}
	else
	{
		axisX->append(categories);
		if (!xlabel_.isEmpty()) axisX->setTitleText(xlabel_);
		axisX->setLabelsAngle(-90);
	}

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

	// delete chart;
}

