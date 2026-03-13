#include "Histogram.h"

#include <cmath>
#include <limits>
#include <algorithm>

#include "Exceptions.h"
#include "BasicStatistics.h"
#include <QStringList>
#include <QApplication>

Histogram::Histogram(double min, double max, double bin_size)
	: min_(min)
	, max_(max)
	, bin_size_(bin_size)
	, bin_sum_(0)
	, alpha_(std::numeric_limits<double>::quiet_NaN())
{
	if (bin_size_<=0)
	{
		THROW(StatisticsException,"Cannot initialize histogram with non-positive bin size!");
	}

	if (min_>=max_)
	{
		THROW(StatisticsException,"Cannot initialize histogram with empty range!");
	}

	bins_.resize(ceil((max_-min_)/bin_size_));
}

void Histogram::inc(double val, bool ignore_bounds_errors)
{
	bins_[binIndex(val, ignore_bounds_errors)]+=1;
	bin_sum_ += 1;
}

void Histogram::inc(const QVector<double> &data, bool ignore_bounds_errors)
{
	for (int i=0; i<data.size(); ++i)
	{
		inc(data[i], ignore_bounds_errors);
	}
}

double Histogram::min() const
{
	return min_;
}

double Histogram::max() const
{
	return max_;
}

double Histogram::binSize() const
{
	return bin_size_;
}

int Histogram::binCount() const
{
	return bins_.size();
}

long long Histogram::binSum()
{
	return bin_sum_;
}

double Histogram::maxValue(bool as_percentage) const
{
	if (bins_.size()==0)
	{
		THROW(StatisticsException,"No bins present!");
	}

	double max = *(std::max_element(bins_.begin(), bins_.end()));
	if(as_percentage)
	{
		return 100.0 * max / (double)bin_sum_;
	}
	return max;
}

double Histogram::minValue(bool as_percentage) const
{
	if (bins_.size()==0)
	{
		THROW(StatisticsException,"No bins present!");
	}

	double min = *(std::min_element(bins_.begin(), bins_.end()));
	if(as_percentage)
	{
		return 100.0 * min / (double)bin_sum_;
	}
	return min;
}

double Histogram::binValue(int index, bool as_percentage) const
{
	if (index<0 || index>=(int)bins_.size())
	{
		THROW(StatisticsException,"Index " + QString::number(index) + " out of range (0-" + QString::number(bins_.size()-1) + ")!");
	}

	double value = bins_[index];
	if(as_percentage)
	{
		return 100.0 * value / (double)bin_sum_;
	}
	return value;
}

double Histogram::startOfBin(int index) const
{
	if (index<0 || index>=(int)bins_.size())
	{
		THROW(StatisticsException,"Index " + QString::number(index) + " out of range (0-" + QString::number(bins_.size()-1) + ")!");
	}

	return bin_size_*index + min_;
}

double Histogram::binValue(double val, bool as_percentage, bool ignore_bounds_errors) const
{
	double value = bins_[binIndex(val, ignore_bounds_errors)];
	if(as_percentage)
	{
		return 100.0 * value / (double)bin_sum_;
	}
	return value;
}

int Histogram::binIndex(double val, bool ignore_bounds_errors) const
{
	if (!ignore_bounds_errors && (val < min_ || val > max_))
	{
		THROW(StatisticsException, "Requested position '" + QString::number(val) + "' not in range (" + QString::number(min_) + "-" + QString::number(max_) + ")!");
	}

	int index = floor ( (val-min_) / (max_-min_) * bins_.size());

	return BasicStatistics::bound(static_cast<qsizetype>(index), static_cast<qsizetype>(0), static_cast<qsizetype>(bins_.size()-1));
}


void Histogram::print(QTextStream& stream, QString indentation, int position_precision, int data_precision, bool ascending) const
{
	for (int i=0; i<bins_.count(); ++i)
	{
		int index = ascending ? i : bins_.count()-i-1;
		double start = startOfBin(index);
		double end = start + bin_size_;
		if (!ascending) std::swap(start, end);
		stream << indentation << QString::number(start, 'f', position_precision) << "-" << QString::number(end, 'f', position_precision) << ": " << QString::number(binValue(index), 'f', data_precision) << "\n";
	}
}

QVector<double> Histogram::xCoords()
{
	return BasicStatistics::range(binCount(), startOfBin(0) + 0.5 * binSize(), binSize());
}

QVector<double> Histogram::yCoords(bool as_percentage)
{
	if (as_percentage)
	{
		QVector<double> tmp(bins_);
		for (int i=0; i<tmp.count(); ++i)
		{
			tmp[i] = 100.0 * tmp[i] / bin_sum_;
		}
		return tmp;
	}
	else
	{
		return bins_;
	}
}

void Histogram::store(QString filename, bool x_log_scale, bool y_log_scale, double min_offset)
{
	QVector<double> x = xCoords();
	QVector<double> y = yCoords();

	QBarSet *set = new QBarSet("histogram");

	double y_min = minValue();
	double x_min = min();

	for(int i = 0; i < y.size(); ++i)
	{
		double value = y[i];

		if(y_log_scale && value == 0.0)
			value += min_offset;

		*set << value;
	}

	QBarSeries *series = new QBarSeries();
	series->append(set);

	QChart *chart = new QChart();
	chart->addSeries(series);
	chart->legend()->hide();

	// if(!ylabel_.isEmpty()) chart->setTitle(ylabel_);

	// ---- X axis ----
	QAbstractAxis *axisX;

	if(x_log_scale)
	{
		QLogValueAxis *logAxis = new QLogValueAxis();
		logAxis->setBase(10);
		logAxis->setMinorTickCount(-1);

		if(x_min == 0.0) x_min += min_offset;
		logAxis->setRange(x_min, max());

		axisX = logAxis;
	}
	else
	{
		QValueAxis *valueAxis = new QValueAxis();
		valueAxis->setRange(x_min, max());

		axisX = valueAxis;
	}

	// if(!xlabel_.isEmpty()) axisX->setTitleText(xlabel_);

	chart->addAxis(axisX, Qt::AlignBottom);
	series->attachAxis(axisX);


	// ---- Y axis ----
	QAbstractAxis *axisY;

	if(y_log_scale)
	{
		QLogValueAxis *logAxis = new QLogValueAxis();
		logAxis->setBase(10);

		if(y_min == 0.0) y_min += min_offset;
		logAxis->setRange(y_min, maxValue() + 0.2 * maxValue());

		axisY = logAxis;
	}
	else
	{
		QValueAxis *valueAxis = new QValueAxis();
		valueAxis->setRange(y_min, maxValue() + 0.2 * maxValue());
		axisY = valueAxis;
	}


	if (!ylabel_.isEmpty())
		axisY->setTitleText(ylabel_);

	chart->addAxis(axisY, Qt::AlignLeft);
	series->attachAxis(axisY);


	// ---- X labels (bins) ----
	QStringList categories;
	for(double val : x)
	{
		categories << QString::number(static_cast<int>(std::round(val)));
	}

	QBarCategoryAxis *axisCat = new QBarCategoryAxis();
	axisCat->append(categories);
	if (!xlabel_.isEmpty()) axisCat->setTitleText(xlabel_);

	// QFont font = axisCat->labelsFont();
	// font.setPointSize(12);   // change font size
	// axisCat->setLabelsFont(font);
	chart->setAxisX(axisCat, series);


	// render
	QChartView chartView(chart);
	chartView.resize(1000, 400); // 10x4 inches @ 100 dpi

	chartView.setRenderHint(QPainter::Antialiasing, true);
	chartView.setRenderHint(QPainter::TextAntialiasing, true);
	chartView.setRenderHint(QPainter::SmoothPixmapTransform, true);

	QApplication::processEvents();
	QPixmap pixmap = chartView.grab();

	if (!pixmap.save(filename.replace("\\", "/"), "PNG"))
	{
		THROW(ProgrammingException, "Could not save histogram to file: " + filename);
	}
}

void Histogram::storeCombinedHistogram(QString filename, QList<Histogram> histograms, QString xlabel, QString ylabel)
{
	//check that all histograms have the same bins and labels
	double min = 0;
	double max = 0;
	double minValue = 0;
	double maxValue = 0;
	foreach(Histogram h, histograms)
	{
		if(min > h.min())	min = h.min();
		if(max < h.max())	max = h.max();
		if(minValue > h.minValue())	minValue = h.minValue();
		if(maxValue < h.maxValue())	maxValue = h.maxValue();
	}

	QChart* chart = new QChart();
	chart->legend()->setVisible(true);
	chart->legend()->setAlignment(Qt::AlignRight);
	chart->legend()->hide();

	// if (!xlabel.isEmpty()) chart->setTitle(xlabel);

	QBarSeries* series = new QBarSeries();
	QStringList categories;

	// assume all histograms share the same bins
	if (!histograms.isEmpty())
	{
		QVector<double> x = histograms.first().xCoords();
		for (double v : x) categories << QString::number(v);
	}

	//create one bar set per histogram
	foreach (Histogram h,  histograms)
	{
		QBarSet* set = new QBarSet(h.label_);
		QVector<double> y = h.yCoords();
		for (double v : y) *set << v;

		// color
		if (!h.color_.isEmpty()) set->setColor(QColor(h.color_));

		// alpha
		if (BasicStatistics::isValidFloat(h.alpha_))
		{
			QColor c = set->color();
			c.setAlphaF(h.alpha_);
			set->setColor(c);
		}

		series->append(set);
	}

	chart->addSeries(series);

	// X axis
	QBarCategoryAxis* axisX = new QBarCategoryAxis();
	// axisX->append(categories);
	if (!xlabel.isEmpty()) axisX->setTitleText(xlabel);

	// Y axis
	QValueAxis* axisY = new QValueAxis();
	axisY->setRange(minValue, maxValue + 0.1 * maxValue);
	if (!ylabel.isEmpty()) axisY->setTitleText(ylabel);

	chart->addAxis(axisX, Qt::AlignBottom);
	chart->addAxis(axisY, Qt::AlignLeft);

	series->attachAxis(axisX);
	series->attachAxis(axisY);

	// render
	QChartView chartView(chart);
	chartView.resize(1000, 400);

	chartView.setRenderHint(QPainter::Antialiasing, true);
	chartView.setRenderHint(QPainter::TextAntialiasing, true);
	chartView.setRenderHint(QPainter::SmoothPixmapTransform, true);

	QApplication::processEvents();
	QPixmap pixmap = chartView.grab();

	if (!pixmap.save(filename.replace("\\", "/"), "PNG"))
	{
		THROW(ProgrammingException, "Could not save bar plot to file: " + filename);
	}
}

void Histogram::setYLabel(QString ylabel)
{
	ylabel_ = ylabel;
}

void Histogram::setXLabel(QString xlabel)
{
	xlabel_ = xlabel;
}

void Histogram::setLabel(QString label)
{
	label_ = label;
}

void Histogram::setColor(QString color)
{
	color_ = color;
}

void Histogram::setAlpha(double alpha)
{
	alpha_ = alpha;
}

