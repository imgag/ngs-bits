#include "AnalysisTimePlot.h"
#include "NGSD.h"
#include <QChart>
#include <QScatterSeries>
#include <QApplication>
#include <QDateTime>
#include <QDateTimeAxis>
#include <QValueAxis>
#include "GUIHelper.h"

AnalysisTimePlot::AnalysisTimePlot(QWidget* parent)
	: QWidget(parent)
	, ui_()
{
	ui_.setupUi(this);
	connect(ui_.update_btn, &QPushButton::clicked, this, &AnalysisTimePlot::updatePlot);

	//get enums from NGSD
	NGSD db;
	QStringList types = db.getEnum("analysis_time", "type");
	ui_.type->addItems(types);
	ui_.type->setCurrentIndex(0);
	QStringList sys_types = db.getEnum("processing_system", "type");
	sys_types.prepend("");
	ui_.system_type->addItems(sys_types);
	ui_.system_name->fill(db.createTable("processing_system", "SELECT sys.id, sys.name_manufacturer FROM processing_system sys, genome g WHERE sys.genome_id=g.id AND g.build='GRCh38' ORDER BY sys.name_manufacturer ASC"));
	QStringList servers = db.getValues("SELECT DISTINCT server FROM analysis_time ORDER BY server ASC");
	servers.prepend("");
	ui_.server->addItems(servers);
	ui_.server->setCurrentIndex(0);
}

void AnalysisTimePlot::updatePlot()
{
	bool min_per_threads = ui_.min_per_threads->isChecked();

	try
	{
		QApplication::setOverrideCursor(Qt::BusyCursor);

		//clear (necessary because settings the chart, releases the ownership of the previous chart)
		QChart* chart_old = ui_.plot->chart();
		ui_.plot->setChart(new QChart());
		delete chart_old;

		//get data
		QVector<qint64> times;
		qint64 times_min = std::numeric_limits<qint64>::max();
		qint64 times_max = -std::numeric_limits<qint64>::max();
		QVector<double> mins;
		double mins_min = 0;
		double mins_max = -std::numeric_limits<double>::max();
		NGSD db;
		SqlQuery query = db.getQuery();
		QStringList constraints;
		constraints << "t.type='" + ui_.type->currentText()+"'";
		if (!ui_.system_type->currentText().isEmpty()) constraints << "sys.type='"+ui_.system_type->currentText()+"'";
		if (!ui_.system_name->currentText().isEmpty()) constraints << "sys.name_manufacturer='"+ui_.system_name->currentText()+"'";
		if (!ui_.server->currentText().isEmpty()) constraints << "t.server='"+ui_.server->currentText()+"'";
		QString dragen = ui_.dragen->currentText();
		if (dragen=="with DRAGEN") constraints << "t.dragen_used=1";
		if (dragen=="without DRAGEN") constraints << "t.dragen_used=0";
		query.exec("SELECT t.datetime, t.min, t.threads FROM analysis_time t, processing_system sys WHERE sys.id=t.processing_system_id AND "+constraints.join(" AND "));
		while(query.next())
		{
			QDateTime time = query.value(0).toDateTime();
			if (time.isValid())
			{
				qint64 time_value = time.toMSecsSinceEpoch();
				times << time_value;
				if (time_value<times_min) times_min = time_value;
				if (time_value>times_max) times_max = time_value;

				double min = query.value(1).toDouble();
				double threads = query.value(2).toDouble();
				double min_value = (min_per_threads ? min/threads : min);
				mins << min_value;
				if (min_value>mins_max) mins_max = min_value;
			}
		}
		ui_.count->setText("count: " + QString::number(times.size()));

		//check that at least one time is given
		if (times.isEmpty())
		{
			QApplication::restoreOverrideCursor();
			return;
		}

		//create series
		QScatterSeries* series = new QScatterSeries();
		series->setMarkerShape( QScatterSeries::MarkerShapeCircle);
		series->setBorderColor(Qt::transparent);
		series->setBrush(Qt::darkBlue);
		series->setOpacity(0.7);
		series->setMarkerSize(6);
		for(int i=0; i<times.count(); ++i)
		{
			series->append(times[i], mins[i]);
		}

		//create chart
		QChart* chart = new QChart();
		chart->legend()->setVisible(false);
		chart->legend()->setAlignment(Qt::AlignBottom);

		//add axes
		QAbstractAxis* x_axis;
		QDateTimeAxis* axis = new QDateTimeAxis();
		axis->setFormat("dd.MM.yyyy");
		axis->setTitleText("Date");
		x_axis = axis;
		chart->addAxis(x_axis, Qt::AlignBottom);

		QValueAxis* y_axis = new QValueAxis();
		y_axis->setTitleText(min_per_threads ? "min/threads" : "min");
		y_axis->setTickCount(8);
		chart->addAxis(y_axis, Qt::AlignLeft);

		//add data series
		chart->addSeries(series);
		series->attachAxis(x_axis);
		series->attachAxis(y_axis);

		//margin X (prevent clipping of data point icons)
		double x_margin = 0.01*(times_max==times_min ? times_max : times_max-times_min);
		if (x_margin>0)
		{
			x_axis->setMin(QDateTime::fromMSecsSinceEpoch(times_min-x_margin));
			x_axis->setMax(QDateTime::fromMSecsSinceEpoch(times_max+x_margin));
		}

		//margin Y (prevent clipping of data point icons)
		double y_margin = 0.01*(mins_max==mins_min ? mins_max : mins_max-mins_min);
		if (y_margin>0)
		{
			y_axis->setMin(mins_min-y_margin);
			y_axis->setMax(mins_max+y_margin);
		}

		//update GUI
		chart->legend()->setMarkerShape(QLegend::MarkerShapeFromSeries);
		ui_.plot->setRenderHint(QPainter::Antialiasing);
		ui_.plot->setChart(chart);

		QApplication::restoreOverrideCursor();
	}
	catch(Exception& e)
	{
		GUIHelper::showException(this, e, "Analysis time plot exception");
	}
}
