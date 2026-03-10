#ifndef HISTOGRAM_H
#define HISTOGRAM_H

#include "cppPLOTS_global.h"
#include <QVector>
#include <QTextStream>

#include <QtCharts/QChartView>
#include <QtCharts/QBarSeries>
#include <QtCharts/QBarSet>
#include <QtCharts/QBarCategoryAxis>
#include <QtCharts/QValueAxis>
#include <QtCharts/QLegend>

///Histogram representation
class CPPPLOTSSHARED_EXPORT Histogram
{
public:
	/// Default constructor
	Histogram(double min, double max, double bin_size);

	/// Increases the bin corresponding to value @p val by one
	void inc(double val, bool ignore_bounds_errors=false)
	{
		bins_[binIndex(val, ignore_bounds_errors)]+=1;
		bin_sum_ += 1;
	}

	/// Increases the bin corresponding to the values in @p data by one
	void inc(const QVector<double>& data, bool ignore_bounds_errors=false)
	{
		for (int i=0; i<data.size(); ++i)
		{
			inc(data[i], ignore_bounds_errors);
		}
	}

	/// Returns the lower bound position (x-axis)
	double min() const
	{
		return min_;
	}
	/// Returns the upper bound position (x-axis)
	double max() const
	{
		return max_;
	}
	/// Returns the bin size
	double binSize() const
	{
		return bin_size_;
	}

	/// Returns the number of bins
	int binCount() const
	{
		return bins_.size();
	}
	/// Returns the sum of all bins (i.e. the number of data points added)
	long long binSum()
	{
		return bin_sum_;
	}

	/// Returns the bin a given position belongs to.
	int binIndex(double val, bool ignore_bounds_errors=false) const;

	/// Returns the highest value of all bins (y-axis)
	double maxValue(bool as_percentage=false) const;
	/// Returns the lowest value of all bins (y-axis)
	double minValue(bool as_percentage=false) const;

	/// Returns the value of the bin corresponding to the position @p val
	double binValue(double val, bool as_percentage=false, bool ignore_bounds_errors=false) const;
	/// Returns the value of the bin with the index @p index
	double binValue(int index, bool as_percentage=false) const;

	/// Returns the start position of the bin with the index @p index
	double startOfBin(int index) const;

	/// Prints the histogram to a stream
	void print(QTextStream &stream, QString indentation="", int position_precision=2, int data_precision=2, bool ascending=true) const;

	/// Returns an array of X-coordinates (position).
	QVector<double> xCoords();
	/// Returns an array of Y-coordinates (values).
	QVector<double> yCoords(bool as_percentage=false);

	/// store
	void store(QString filename, bool x_log_scale=false, bool y_log_scale=false, double min_offset=1e-6);
	/// stores a combined histogram of different histograms
	static void storeCombinedHistogram(QString filename, QList<Histogram> histograms, QString xlabel, QString ylabel);

	void setYLabel(QString ylabel)
	{
		ylabel_ = ylabel;
	}

	void setXLabel(QString xlabel)
	{
		xlabel_ = xlabel;
	}

	void setLabel(QString label)
	{
		label_ = label;
	}

	void setColor(QString color)
	{
		color_ = color;
	}

	void setAlpha(double alpha)
	{
		alpha_ = alpha;
	}

protected:
	/// lower bound position
	double min_;
	/// upper bound position
	double max_;
	/// bin size
	double bin_size_;
	/// sum of all bins (used for percentage mode)
	long long bin_sum_;
	QString xlabel_;
	QString ylabel_;
	QString label_;
	QString color_;
	double alpha_;

	/// vector of bins
	QVector<double> bins_;
};

#endif // HISTOGRAM_H
