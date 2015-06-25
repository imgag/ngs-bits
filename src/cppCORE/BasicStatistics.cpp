#include <algorithm>
#include <numeric>
#include <math.h>
#include <limits>
#include "BasicStatistics.h"
#include "Exceptions.h"

double BasicStatistics::mean(const QVector<double>& data)
{
	if (data.count()==0)
	{
		THROW(StatisticsException, "Cannot calculate mean on empty data array.");
	}

	return std::accumulate(data.begin(), data.end(), 0.0) / data.size();
}

double BasicStatistics::stdev(const QVector<double>& data)
{
	return stdev(data, mean(data));
}

double BasicStatistics::stdev(const QVector<double>& data, double mean)
{
	if (data.count()==0)
	{
		THROW(StatisticsException, "Cannot calculate standard deviation on empty data array.");
	}

	double output = 0.0;

	for (int i=0; i<data.size(); ++i)
	{
		output += pow(data[i]-mean, 2);
	}

	return sqrt(output/data.size());
}

double BasicStatistics::median(const QVector<double>& data)
{
	if (!isSorted(data))
	{
		THROW(StatisticsException, "Cannot calculate median on unsorted data array.");
	}
	if (data.count()==0)
	{
		THROW(StatisticsException, "Cannot calculate median of data array with zero length!");
	}

	int n = data.count();
	if (n%2==0)
	{
		return 0.5 * (data[n/2-1] + data[n/2]);
	}
	else
	{
		return data[n/2];
	}
}

double BasicStatistics::mad(const QVector<double>& data, double median)
{
	QVector<double> devs;
	devs.reserve(data.count());
	foreach(double value, data)
	{
		devs.append(fabs(value-median));
	}
	std::sort(devs.begin(), devs.end());
	return BasicStatistics::median(devs);
}

double BasicStatistics::correlation(const QVector<double>& x, const QVector<double>& y)
{
	if (x.count()!=y.count())
	{
		THROW(StatisticsException, "Cannot calculate correlation of data arrays with different length!");
	}
	if (x.count()==0)
	{
		THROW(StatisticsException, "Cannot calculate correlation of data arrays with zero length!");
	}

	double x_mean = mean(x);
	double y_mean = mean(y);

	double sum = 0.0;
	for(int i=0; i<x.size(); ++i)
	{
		sum += (x[i]-x_mean) * (y[i]-y_mean);
	}

	return sum / stdev(x, x_mean) / stdev(y, y_mean) / x.size();
}

bool BasicStatistics::isValidFloat(double value)
{
	if (value != value)
	{
		return false;
	}
	if (value > std::numeric_limits<double>::max())
	{
		return false;
	}
	if (value < -std::numeric_limits<double>::max())
	{
		return false;
	}

	return true;
}

bool BasicStatistics::isValidFloat(QByteArray value)
{
	bool ok = true;
	double numeric_value = value.toDouble(&ok);
	return ok && isValidFloat(numeric_value);
}

double BasicStatistics::bound(double value, double lower_bound, double upper_bound)
{
	if (value<lower_bound) return lower_bound;
	if (value>upper_bound) return upper_bound;
	return value;
}

bool BasicStatistics::isSorted(const QVector<double>& data)
{
	for (int i=1; i<data.count(); ++i)
	{
		if (data[i-1]>data[i]) return false;
	}

	return true;
}

int BasicStatistics::sign(int val)
{
	return (0<val) - (val<0);
}

QPair<double, double> BasicStatistics::linearRegression(const QVector<double>& x, const QVector<double>& y)
{
	// initializing sum of x and y values
	int count_valid = 0;
	double sum_x = 0.0;
	double sum_y = 0.0;
	for (int i=0; i<x.size(); i++)
	{
		if (isValidFloat(x[i]) && isValidFloat(y[i]))
		{
			sum_x += x[i];
			sum_y += y[i];
			++count_valid;
		}
	}

	// middle index of the section
	double sxoss = sum_x / count_valid;

	// initializing b
	double slope = 0.0;
	double st2 = 0.0;
	for (int i=0; i<x.size(); i++)
	{
		if (isValidFloat(x[i]) && isValidFloat(y[i]))
		{
			// t is distance from the middle index
			double t = x[i]-sxoss;

			// sum of the squares of the distance from the average
			st2 += t*t;

			// b is sum of datapoints weighted by the distance
			slope += t * y[i];
		}
	}

	// averaging b by the maximum distance from the average
	slope /= st2;

	// average difference between data points and distance weighted data points.
	double offset = (sum_y - sum_x*slope) / count_valid;

	//create output
	return qMakePair(offset, slope);
}

QPair<double, double> BasicStatistics::getMinMax(const QVector<double>& data)
{
	double min = std::numeric_limits<double>::max();
	double max = -std::numeric_limits<double>::max();
	foreach(double v, data)
	{
		if (!isValidFloat(v)) continue;

		min = std::min(min, v);
		max = std::max(max, v);
	}

	return qMakePair(min, max);
}

QVector<double> BasicStatistics::range(int size, double start_value, double increment)
{
	double next_val = start_value;

	QVector<double> output;
	while(output.count()<size)
	{
		output << next_val;
		next_val += increment;
	}
	return output;
}


