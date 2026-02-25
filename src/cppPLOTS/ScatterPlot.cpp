#include "ScatterPlot.h"
#include "Exceptions.h"
#include "Log.h"
#include "BasicStatistics.h"
#include <QStringList>
#include <limits>
#include <QApplication>

ScatterPlot::ScatterPlot()
	: yrange_set_(false), xrange_set_(false), yscale_log_(false), noxticks_(false)
{
}

void ScatterPlot::setValues(const QList<std::pair<double,double>>& values, const QList<QString>& colors)
{
	points_.clear();
	points_.append(values);
	colors_.clear();
	colors_.append(colors);
}

void ScatterPlot::setXLabel(QString xlabel)
{
	xlabel_ = xlabel;
}

void ScatterPlot::setYLabel(QString ylabel)
{
	ylabel_ = ylabel;
}

void ScatterPlot::setXRange(double xmin, double xmax)
{
	xmin_ = xmin;
	xmax_ = xmax;
	xrange_set_ = true;
}

void ScatterPlot::setYRange(double ymin, double ymax)
{
	ymin_ = ymin;
	ymax_ = ymax;
	yrange_set_ = true;
}

void ScatterPlot::addColorLegend(QString color, QString desc)
{
	color_legend_.insert(color, desc);
}

void ScatterPlot::addVLine(double x)
{
	vlines_.append(x);
}

void ScatterPlot::store(QString filename)
{
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

	if (!yrange_set_ && !points_.isEmpty())
	{
		double min = std::numeric_limits<double>::max();
		double max = -std::numeric_limits<double>::max();

		for (const auto& p : points_)
		{
			min = std::min(min, p.second);
			max = std::max(max, p.second);
		}

		ymin_ = min - 0.01 * (max - min);
		ymax_ = max + 0.01 * (max - min);
	}

	if (!xrange_set_ && !points_.isEmpty())
	{
		double min = std::numeric_limits<double>::max();
		double max = -std::numeric_limits<double>::max();

		for (const auto& p : points_)
		{
			min = std::min(min, p.first);
			max = std::max(max, p.first);
		}

		xmin_ = min-0.01*(max-min);
		xmax_ = max+0.01*(max-min);
	}

	QChart* chart = new QChart();
	chart->legend()->setVisible(color_legend_.count() > 0);

	// group by color (Qt needs one series per color for legend support)
	QMap<QString, QScatterSeries*> series_by_color;
	for (int i=0; i<points_.size(); ++i)
	{
		const auto& p = points_[i];
		QString color = (colors_.size() > i) ? colors_[i] : "black";

		if (!series_by_color.contains(color))
		{
			auto* s = new QScatterSeries();
			s->setMarkerSize(6.0);
			s->setColor(QColor::fromString(color));

			// legend label if available
			if (color_legend_.contains(color))
			{
				s->setName(color_legend_[color]);
			}
			else
			{
				s->setName("");
			}

			chart->addSeries(s);
			series_by_color[color] = s;
		}

		series_by_color[color]->append(p.first, p.second);
	}

	// vertical lines
	for (double x : vlines_)
	{
		auto* line = new QLineSeries();
		line->append(x, ymin_);
		line->append(x, ymax_);

		QPen pen(Qt::black);
		pen.setStyle(Qt::DashLine);
		line->setPen(pen);
		line->setName("");

		chart->addSeries(line);
	}

	QAbstractAxis* axis_x = new QValueAxis();
	axis_x->setTitleText(xlabel_);

	QAbstractAxis* axis_y = nullptr;

	if (yscale_log_)
	{
		auto* logAxis = new QLogValueAxis();
		logAxis->setTitleText(ylabel_);
		axis_y = logAxis;
	}
	else
	{
		auto* valAxis = new QValueAxis();
		valAxis->setTitleText(ylabel_);
		axis_y = valAxis;
	}

	chart->addAxis(axis_x, Qt::AlignBottom);
	chart->addAxis(axis_y, Qt::AlignLeft);

	// attach axes
	for (auto* s : chart->series())
	{
		s->attachAxis(axis_x);
		s->attachAxis(axis_y);
	}

	if (BasicStatistics::isValidFloat(xmin_) && BasicStatistics::isValidFloat(xmax_))
	{
		static_cast<QValueAxis*>(axis_x)->setRange(xmin_, xmax_);
	}

	if (BasicStatistics::isValidFloat(ymin_) && BasicStatistics::isValidFloat(ymax_))
	{
		if (yscale_log_)
			static_cast<QLogValueAxis*>(axis_y)->setRange(ymin_, ymax_);
		else
			static_cast<QValueAxis*>(axis_y)->setRange(ymin_, ymax_);
	}

	// image rendering
	QChartView chartView(chart);
	chartView.resize(600, 400);

	// antialiasing for smoother lines and text
	chartView.setRenderHint(QPainter::Antialiasing, true);
	chartView.setRenderHint(QPainter::TextAntialiasing, true);
	chartView.setRenderHint(QPainter::SmoothPixmapTransform, true);

	QApplication::processEvents();
	QPixmap pixmap = chartView.grab();

	if (!pixmap.save(filename.replace("\\", "/"), "PNG"))
		THROW(ProgrammingException, "Could not save bar plot to file: " + filename);

	delete chart;
}
