#include "LinePlot.h"
#include "Exceptions.h"
#include "Log.h"
#include "BasicStatistics.h"
#include <QStringList>
#include <limits>
#include <QApplication>

LinePlot::LinePlot()
	: yrange_set_(false)
{
}

void LinePlot::addLine(const QVector<double>& values, QString label)
{
	if (xvalues_.count()!=0 && values.count()!=xvalues_.count())
	{
		THROW(ArgumentException, "Plot '" + label + "' has " + QString::number(values.count()) + " values, but " + QString::number(xvalues_.count()) + " are expected because x axis values are set!");
	}

	lines_.append(PlotLine(values, label));
}

void LinePlot::setXValues(const QVector<double>& xvalues)
{
	if (lines_.count()!=0)
	{
		THROW(ProgrammingException, "You have to set x axis values of LinePlot before adding any lines!");
	}

	xvalues_ = xvalues;
}

void LinePlot::setXLabel(QString xlabel)
{
	xlabel_ = xlabel;
}

void LinePlot::setYLabel(QString ylabel)
{
	ylabel_ = ylabel;
}

void LinePlot::setYRange(double ymin, double ymax)
{
	ymin_ = ymin;
	ymax_ = ymax;
	yrange_set_ = true;
}

void LinePlot::store(QString filename)
{
	if (lines_.isEmpty())
	{
		Log::warn("LinePlot::store(): No lines to plot.");
		return;
	}

	// --- determine Y range (same logic as your Python version) ---
	if (!yrange_set_)
	{
		double minVal = std::numeric_limits<double>::max();
		double maxVal = -std::numeric_limits<double>::max();

		for (const PlotLine& line : lines_)
		{
			for (double value : line.values)
			{
				minVal = std::min(value, minVal);
				maxVal = std::max(value, maxVal);
			}
		}

		if (maxVal > minVal)
		{
			ymin_ = minVal - 0.01 * (maxVal - minVal);
			ymax_ = maxVal + 0.01 * (maxVal - minVal);
		}
	}

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

	// create axes
	QValueAxis* axisX = new QValueAxis();
	if (!xlabel_.isEmpty()) axisX->setTitleText(xlabel_);
	chart->addAxis(axisX, Qt::AlignBottom);

	QValueAxis* axisY = new QValueAxis();
	if (!ylabel_.isEmpty()) axisY->setTitleText(ylabel_);
	if (BasicStatistics::isValidFloat(ymin_) && BasicStatistics::isValidFloat(ymax_))
	{
		axisY->setRange(ymin_, ymax_);
	}
	chart->addAxis(axisY, Qt::AlignLeft);

	// series of data points
	for (const PlotLine& line : lines_)
	{
		QLineSeries* series = new QLineSeries();
		series->setName(line.label);

		int n = line.values.size();
		for (int i = 0; i < n; ++i)
		{
			double x = (i < xvalues_.size()) ? xvalues_[i] : i;
			series->append(x, line.values[i]);
		}

		chart->addSeries(series);
		series->attachAxis(axisX);
		series->attachAxis(axisY);

		// improves line smoothness (optional) to look very close to matplotlib
		QPen pen = series->pen();
		pen.setWidthF(1.5);
		pen.setCapStyle(Qt::RoundCap);
		pen.setJoinStyle(Qt::RoundJoin);
		series->setPen(pen);
	}

	// title and legend
	if (lines_.count() == 1)
	{
		if (!lines_[0].label.isEmpty())
		{
			chart->setTitle(lines_[0].label);
		}
		chart->legend()->setVisible(false);
	}
	else
	{
		chart->legend()->setVisible(true);
		chart->legend()->setAlignment(Qt::AlignBottom);

		QFont legendFont = chart->legend()->font();
		legendFont.setPointSize(8);
		chart->legend()->setFont(legendFont);
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
	{
		THROW(ProgrammingException, "Could not save plot to file: " + filename);
	}

	//delete chart; // prevent leak
}


LinePlot::PlotLine::PlotLine()
{
}

LinePlot::PlotLine::PlotLine(const QVector<double>& v, QString l)
	: values(v)
	, label(l)
{
}
