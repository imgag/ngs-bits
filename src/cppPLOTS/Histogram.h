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
#include <QtCharts/QLogValueAxis>
#include <QtCharts/QLegend>

///Histogram representation
class CPPPLOTSSHARED_EXPORT Histogram
{
public:
	/// Default constructor
	Histogram(double min, double max, double bin_size);

	/// Increases the bin corresponding to value @p val by one
	void inc(double val, bool ignore_bounds_errors=false);

	/// Increases the bin corresponding to the values in @p data by one
	void inc(const QVector<double>& data, bool ignore_bounds_errors=false);

	/// Returns the lower bound position (x-axis)
	double min() const;
	/// Returns the upper bound position (x-axis)
	double max() const;
	/// Returns the bin size
	double binSize() const;

	/// Returns the number of bins
	int binCount() const;
	/// Returns the sum of all bins (i.e. the number of data points added)
	long long binSum();

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

	void setYLabel(QString ylabel);

	void setXLabel(QString xlabel);

	void setLabel(QString label);

	void setColor(QString color);

	void setAlpha(double alpha);

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
