#include "LinePlot.h"
#include "Exceptions.h"
#include "Helper.h"
#include "Log.h"
#include "BasicStatistics.h"
#include <QStringList>
#include <limits>
#include <QProcess>
#include <QStandardPaths>

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
	//check if python is installed
	QString python_exe = QStandardPaths::findExecutable("python");
	if (python_exe=="")
	{
		Log::warn("Python executable not found in PATH - skipping plot generation!");
		return;
	}

	//create python script
	QString scriptfile = Helper::tempFileName(".py");
	QStringList script;
	script.append("import matplotlib as mpl");
	script.append("mpl.use('Agg')");
	script.append("import matplotlib.pyplot as plt");
	script.append("plt.figure(figsize=(6, 4), dpi=100)");
	if(ylabel_!="") script.append("plt.ylabel('" + ylabel_ + "')");
	if(xlabel_!="") script.append("plt.xlabel('" + xlabel_ + "')");
	if(!yrange_set_)
	{
		double min = std::numeric_limits<double>::max();
		double max = -std::numeric_limits<double>::max();
		foreach(const PlotLine& line, lines_)
		{
			foreach(double value, line.values)
			{
				min = std::min(value, min);
				max = std::max(value, max);
			}
		}
		ymin_ = min-0.01*(max-min);
		ymax_ = max+0.01*(max-min);;
	}
	if(BasicStatistics::isValidFloat(ymin_) && BasicStatistics::isValidFloat(ymax_))
	{
		script.append("plt.ylim(" + QString::number(ymin_) + "," + QString::number(ymax_) + ")");
	}
	QString xvaluestring = "";
	if (xvalues_.count()>0)
	{
		xvaluestring += "[" + QString::number(xvalues_[0]);
		for (int i=1; i<xvalues_.count(); ++i)
		{
			xvaluestring += ","+QString::number(xvalues_[i]);
		}
		xvaluestring += "],";
	}
	foreach(const PlotLine& line, lines_)
	{
		QString valuestring = "[";
		if (line.values.count()>0)
		{
			valuestring += QString::number(line.values[0]);
			for (int i=1; i<line.values.count(); ++i)
			{
				valuestring += ","+QString::number(line.values[i]);
			}
		}
		valuestring += "]";
		script.append("plt.plot(" + xvaluestring + valuestring + ", label='" + line.label + "')");

	}
	if(lines_.count()==1)
	{
		if (lines_[0].label!="")
		{
			script.append("plt.title('" + lines_[0].label + "')");
		}
	}
	else
	{
		script.append("plt.legend(prop={'size':8}, labelspacing=0.2)");
	}
	script.append("plt.savefig(\"" + filename.replace("\\", "/") + "\", bbox_inches=\'tight\', dpi=100)");
	Helper::storeTextFile(scriptfile, script);

	//execute scipt
	QProcess process;
	process.setProcessChannelMode(QProcess::MergedChannels);
	process.start(python_exe, QStringList() << scriptfile);
	if (!process.waitForFinished(-1) || process.readAll().contains("rror"))
	{
		THROW(ProgrammingException, "Could not execute python script! Error message is: " + process.errorString());
	}

	//remove temporary file
	QFile::remove(scriptfile);
}


LinePlot::PlotLine::PlotLine()
{
}

LinePlot::PlotLine::PlotLine(const QVector<double>& v, QString l)
	: values(v)
	, label(l)
{
}
