#include "DBQCWidget.h"
#include "BasicStatistics.h"

#include <QDebug>

DBQCWidget::DBQCWidget(QWidget *parent)
	: QWidget(parent)
	, ui_()
{
	ui_.setupUi(this);

	//fill compo boxes
	ui_.term->fill(db_.createTable("qc_terms", "SELECT id, name FROM qc_terms WHERE obsolete=0 ORDER BY qcml_id ASC"));
	connect(ui_.term, SIGNAL(currentTextChanged(QString)), this, SLOT(updateGUI()));
	ui_.system->fill(db_.createTable("processing_system", "SELECT id, name_manufacturer FROM processing_system ORDER BY name_manufacturer ASC"));
	connect(ui_.system, SIGNAL(currentTextChanged(QString)), this, SLOT(updateGUI()));
}

void DBQCWidget::setSystemId(QString id)
{
	ui_.system->setCurrentId(id);
}

void DBQCWidget::setTermId(QString id)
{
	ui_.term->setCurrentId(id);
}

void DBQCWidget::updateGUI()
{
	//clear (necessary because settings the chart, releases the ownership of the previous chart)
	QChart* chart_old = ui_.plot->chart();
	ui_.plot->setChart(new QChart());
	delete chart_old;

	QString term_id = ui_.term->getCurrentId();
	if (term_id.isEmpty())
	{
		return;
	}

	QApplication::setOverrideCursor(Qt::BusyCursor);

	//create query
	QString query_string = "SELECT qc.value, ps.quality, r.end_date FROM processed_sample_qc qc, processed_sample ps, sequencing_run r WHERE qc.processed_sample_id=ps.id AND ps.sequencing_run_id=r.id AND qc.qc_terms_id='" + term_id + "'";
	QString sys_id = ui_.system->getCurrentId();
	if (!sys_id.isEmpty())
	{
		query_string += " AND ps.processing_system_id='" + sys_id + "'";
	}

	//execute query
	SqlQuery query = db_.getQuery();
	query.exec(query_string);

	//perform statistics
	QVector<double> values;
	values.reserve(query.size());
	while(query.next())
	{
		bool ok;
		double value = query.value(0).toDouble(&ok);
		if (ok && query.value(1).toString()!="bad")
		{
			values << value;
		}
	}
	int count = values.size();
	ui_.count->setText(QString::number(count));
	if (count>0)
	{
		double mean = BasicStatistics::mean(values);
		double stdev = BasicStatistics::stdev(values, mean);
		ui_.mean->setText(QString::number(mean));
		ui_.stdev->setText(QString::number(stdev));
	}
	else
	{
		ui_.mean->setText("n/a");
		ui_.stdev->setText("n/a");
	}

	//create series (colored by quality)
	QHash<QString, QScatterSeries*> series;
	series.insert("good", getSeries(QColor("#11A50F")));
	series.insert("medium", getSeries(QColor("#FF9600")));
	series.insert("bad", getSeries(QColor("#FF0000")));
	series.insert("n/a", getSeries(Qt::gray));
	double value_min = std::numeric_limits<double>::max();
	double value_max = -std::numeric_limits<double>::max();
	double date_min = std::numeric_limits<double>::max();
	double date_max = -std::numeric_limits<double>::max();
	query.seek(-1);
	while(query.next())
	{
		bool ok;
		double value = query.value(0).toDouble(&ok);
		if (!ok) continue; //value not a number
		if (value<value_min) value_min = value;
		if (value>value_max) value_max = value;

		if (query.value(2).isNull()) continue; //date null
		double date = query.value(2).toDateTime().toMSecsSinceEpoch();
		if (date<date_min) date_min = date;
		if (date>date_max) date_max = date;

		QString quality = query.value(1).toString();
		if (!series.contains(quality))
		{
			THROW(ProgrammingException, "Invalid processed sample quality '" + quality + "' in DBQCWidget::updateGUI!");
		}
		series[quality]->append(date, value);
	}

	//create chart
	QChart* chart = new QChart();
	chart->legend()->setVisible(false);

	QDateTimeAxis* x_axis = new QDateTimeAxis;
	x_axis->setFormat("dd.MM.yyyy");
	x_axis->setTitleText("Date");
	chart->setAxisX(x_axis);

	QValueAxis* y_axis = new QValueAxis;
	y_axis->setTitleText(ui_.term->currentText());
	y_axis->setTickCount(8);
	chart->setAxisY(y_axis);

	foreach(QScatterSeries* s, series)
	{
		if (s->count()!=0)
		{
			chart->addSeries(s);
			s->attachAxis(x_axis);
			s->attachAxis(y_axis);
		}
	}

	//add 1% margin to prevent clipping of data point icons
	double y_margin = 0.01*(value_max-value_min);
	double x_margin = 0.01*(date_max-date_min);
	if (x_margin>0 && y_margin>0)
	{
		x_axis->setMin(QDateTime::fromMSecsSinceEpoch(date_min-x_margin));
		x_axis->setMax(QDateTime::fromMSecsSinceEpoch(date_max+x_margin));
		y_axis->setMin(value_min-y_margin);
		y_axis->setMax(value_max+y_margin);
	}

	//update GUI
	ui_.plot->setRenderHint(QPainter::Antialiasing);
	ui_.plot->setChart(chart);

	QApplication::restoreOverrideCursor();
}

QScatterSeries* DBQCWidget::getSeries(QColor color)
{
	QScatterSeries* series = new QScatterSeries();

	series->setMarkerShape(QScatterSeries::MarkerShapeCircle);
	series->setBorderColor(Qt::transparent);
	series->setBrush(color);
	series->setOpacity(0.5);
	series->setMarkerSize(6);

	return series;
}
