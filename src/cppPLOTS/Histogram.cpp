#include "Histogram.h"

// #include <cmath>
#include <limits>
// #include <algorithm>

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

	QBarSeries* series = new QBarSeries();
	QBarSet* set = new QBarSet("");

	QStringList categories;

	for (int i = 0; i < x.size(); ++i)
	{
		*set << y[i];                          // bin height
		categories << QString::number(x[i]);   // bin label
	}

	series->append(set);

	QChart* chart = new QChart();
	chart->addSeries(series);
	chart->legend()->hide();

	if (!xlabel_.isEmpty())
		chart->setTitle(xlabel_);

	// X axis
	QBarCategoryAxis* axisX = new QBarCategoryAxis();
	axisX->append(categories);
	if (!xlabel_.isEmpty())
		axisX->setTitleText(xlabel_);

	// Y axis
	double y_min = minValue();
	double y_max = maxValue() + 0.2 * maxValue();

	QValueAxis* axisY = new QValueAxis();
	axisY->setRange(y_min, y_max);

	if (!ylabel_.isEmpty())
		axisY->setTitleText(ylabel_);

	chart->addAxis(axisX, Qt::AlignBottom);
	chart->addAxis(axisY, Qt::AlignLeft);

	series->attachAxis(axisX);
	series->attachAxis(axisY);


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




	//file handling
	// script.append("plt.savefig('" + filename.replace("\\", "/") + "', bbox_inches=\'tight\', dpi=100)");

	//check if python is installed
	// QString python_exe = Settings::path("python_exe", true);
	// if (python_exe=="") python_exe  = QStandardPaths::findExecutable("python3");
	// if (python_exe!="")
	// {
	// 	QString scriptfile = Helper::tempFileName(".py");
	// 	Helper::storeTextFile(scriptfile, script);

	// 	//execute scipt
	// 	QProcess process;
	// 	process.setProcessChannelMode(QProcess::MergedChannels);
	// 	process.start(python_exe, QStringList() << scriptfile);
	// 	bool success = process.waitForFinished(-1);
	// 	QByteArray output = process.readAll();
	// 	if (!success || process.exitCode()>0)
	// 	{
	// 		THROW(ProgrammingException, "Could not execute python script:\n" + scriptfile + "\n Error message is: " + output);
	// 	}

	// 	//remove temporary file
	// 	QFile::remove(scriptfile);
	// }
	// else
	// {
	// 	Log::warn("Python executable not found in PATH - skipping plot generation!");
	// }
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

	if (!xlabel.isEmpty())
		chart->setTitle(xlabel);

	QBarSeries* series = new QBarSeries();

	QStringList categories;

	// assume all histograms share the same bins
	if (!histograms.isEmpty())
	{
		QVector<double> x = histograms.first().xCoords();
		for (double v : x)
			categories << QString::number(v);
	}

	// create one bar set per histogram
	for (Histogram& h : histograms)
	{
		QBarSet* set = new QBarSet(h.label_);

		QVector<double> y = h.yCoords();
		for (double v : y)
			*set << v;

		// color
		if (!h.color_.isEmpty())
			set->setColor(QColor(h.color_));

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
	axisX->append(categories);
	if (!xlabel.isEmpty())
		axisX->setTitleText(xlabel);

	// Y axis
	QValueAxis* axisY = new QValueAxis();
	axisY->setRange(minValue, maxValue + 0.1 * maxValue);
	if (!ylabel.isEmpty())
		axisY->setTitleText(ylabel);

	chart->addAxis(axisX, Qt::AlignBottom);
	chart->addAxis(axisY, Qt::AlignLeft);

	series->attachAxis(axisX);
	series->attachAxis(axisY);


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


	//create python script
	// QStringList script;
	// script.append("import matplotlib as mpl");
	// script.append("mpl.use('Agg')");
	// script.append("import matplotlib.pyplot as plt");
	// script.append("plt.figure(figsize=(10, 4), dpi=100)");
	// if(ylabel!="") script.append("plt.ylabel('" + ylabel + "')");
	// if(xlabel!="") script.append("plt.xlabel('" + xlabel + "')");

	// script.append("plt.tick_params(axis='x', which='both', bottom='off', top='off')");
	// script.append("plt.tick_params(axis='y', which='both', left='off', right='off')");
	// script.append("plt.ylim(" + QString::number(minValue) + "," + QString::number(maxValue + 0.1*maxValue) + ")");
	// script.append("plt.xlim(" + QString::number(min) + "," + QString::number(max) + ")");

	// //data
	// foreach(Histogram h, histograms)
	// {
	// 	QString yvaluestring = "";
	// 	QString xvaluestring = QString::number(h.min());
	// 	QVector<double> x = h.xCoords();
	// 	QVector<double> y = h.yCoords();
	// 	for (int i=0; i<x.size(); ++i)
	// 	{
	// 		for(int j=0; j<y[i];++j)
	// 		{
	// 			yvaluestring += "," + QString::number(x[i]);
	// 		}
	// 		xvaluestring += "," + QString::number(x[i]+0.5*h.binSize());
	// 	}
	// 	xvaluestring = "[" + xvaluestring + "]";	//remove first ','
	// 	yvaluestring = "[" + yvaluestring.remove(0,1) + "]";

	// 	script.append("plt.hist(" + yvaluestring + ", bins=" + xvaluestring + ", rwidth = 0.8, " + (h.color_.size()>0 ? "color=" + h.color_ + ", " : "") + (BasicStatistics::isValidFloat(h.alpha_) ? "alpha=" + QString::number(h.alpha_) + "," : "") + " edgecolor='none', label = '" + h.label_ + "')");
	// }
	// script.append("plt.legend(loc='upper right',fontsize=10)");

	// //file handling
	// script.append("plt.savefig('" + filename.replace("\\", "/") + "', bbox_inches=\'tight\', dpi=100)");

	//check if python is installed
	// QString python_exe = Settings::path("python_exe", true);
	// if (python_exe=="") python_exe  = QStandardPaths::findExecutable("python3");
	// if (python_exe!="")
	// {
	// 	QString scriptfile = Helper::tempFileName(".py");
	// 	Helper::storeTextFile(scriptfile, script);

	// 	//execute scipt
	// 	QProcess process;
	// 	process.setProcessChannelMode(QProcess::MergedChannels);
	// 	process.start(python_exe, QStringList() << scriptfile);
	// 	bool success = process.waitForFinished(-1);
	// 	QByteArray output = process.readAll();
	// 	if (!success || process.exitCode()>0)
	// 	{
	// 		THROW(ProgrammingException, "Could not execute python script:\n" + scriptfile + "\n Error message is: " + output);
	// 	}

	// 	//remove temporary file
	// 	QFile::remove(scriptfile);
	// }
	// else
	// {
	// 	Log::warn("Python executable not found in PATH - skipping plot generation!");
	// }
}

