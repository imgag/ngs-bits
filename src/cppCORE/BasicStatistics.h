#ifndef BASICSTATISTICS_H
#define BASICSTATISTICS_H

#include "cppCORE_global.h"
#include <QVector>
#include <QPair>

///Statistics helper class.
class CPPCORESHARED_EXPORT BasicStatistics
{
public:
	///Calculates the mean of the data.
	static double mean(const QVector<double>& data);
	///Calculates the standard deviation of the data.
	static double stdev(const QVector<double>& data);
	///Calculates the standard deviation of the data, using a given mean.
	static double stdev(const QVector<double>& data, double mean);
	///Calculates the median of the data. The input data must be sorted!
	static double median(const QVector<double>& data);
	///Calculates the median average deviation (multiply by 1.428 to get a robust estimator of the stdev).
	static double mad(const QVector<double>& data, double median);
	///Calculates the correlation of two data arrays.
	static double correlation(const QVector<double>& x, const QVector<double>& y);

	///Returns if a float is valid.
	static bool isValidFloat(double value);
	///Returns if a string is a representations of a valid float.
	static bool isValidFloat(QByteArray value);
	///Returns the value bounded to the given range.
	static double bound(double value, double lower_bound, double upper_bound);
	///Returns if the data is sorted.
	static bool isSorted(const QVector<double>& data);
	///Returns the sign of an integer, or 0 for 0.
	static int sign(int val);

	///Returns the offset and slope of a linear regression. Ignores invalid values.
	static QPair<double, double> linearRegression(const QVector<double>& x, const QVector<double>& y);
	///Returns minimum and maximum of a dataset. Ignores invalid values.
	static QPair<double, double> getMinMax(const QVector<double>& data);

	///Returns an even-spaced range of values.
	static QVector<double> range(int size, double start_value, double increment);
};

#endif
