#include "DBQCWidget.h"
#include "BasicStatistics.h"
#include "DBSelector.h"
#include "GUIHelper.h"
#include "ProcessedSampleSelector.h"

#include <QDebug>

DBQCWidget::DBQCWidget(QWidget *parent)
	: QWidget(parent)
	, ui_()
{
	ui_.setupUi(this);
	ui_.plot->setRubberBand(QChartView::RectangleRubberBand);
	connect(ui_.reset_zoom, SIGNAL(clicked(bool)), this, SLOT(resetZoom()));
	connect(ui_.swap_metrics, SIGNAL(clicked(bool)), this, SLOT(swapMetrics()));
	QMenu* menu = new QMenu(this);
	menu->addAction("Add sample", this, SLOT(addHighlightSample()));
	menu->addAction("Add run", this, SLOT(addHighlightRun()));
	menu->addAction("Add project", this, SLOT(addHighlightProject()));
	menu->addSeparator();
	menu->addAction("Clear", this, SLOT(clearHighlighting()));
	ui_.highlight_btn->setMenu(menu);

	//fill compo boxes
	ui_.term->fill(db_.createTable("qc_terms", "SELECT id, name FROM qc_terms WHERE obsolete=0 ORDER BY qcml_id ASC"));
	connect(ui_.term, SIGNAL(currentTextChanged(QString)), this, SLOT(updatePlot()));
	ui_.term2->fill(db_.createTable("qc_terms", "SELECT id, name FROM qc_terms WHERE obsolete=0 ORDER BY qcml_id ASC"));
	connect(ui_.term2, SIGNAL(currentTextChanged(QString)), this, SLOT(updatePlot()));
	ui_.system->fill(db_.createTable("processing_system", "SELECT id, name_manufacturer FROM processing_system ORDER BY name_manufacturer ASC"));
	connect(ui_.system, SIGNAL(currentTextChanged(QString)), this, SLOT(updatePlot()));
	connect(ui_.highlight, SIGNAL(editingFinished()), this, SLOT(checkHighlightedSamples()));
	connect(ui_.export_btn, SIGNAL(clicked(bool)), this, SLOT(copyQcMetricsToClipboard()));
	connect(ui_.sample_type, SIGNAL(currentTextChanged(QString)), this, SLOT(updatePlot()));
}

void DBQCWidget::setSystemId(QString id)
{
	ui_.system->setCurrentId(id);
}

void DBQCWidget::addHighlightedProcessedSampleById(QString id, QString name, bool update_plot)
{
	//add ID to set
	highlight_ << id;

	//add name to GUI
	QString text = ui_.highlight->text().trimmed();
	if (!text.isEmpty()) text += " ";
	if (name.isEmpty())
	{
		text += db_.processedSampleName(id);
	}
	else
	{
		text += name.trimmed();
	}
	ui_.highlight->setText(text);

	//update plot
	if (update_plot)
	{
		updatePlot();
	}
}

void DBQCWidget::clearHighlighting()
{
	ui_.highlight->clear();
	ui_.highlight->setStyleSheet("QLineEdit {}");
	ui_.highlight->setToolTip("");

	highlight_.clear();

	updatePlot();
}

void DBQCWidget::addHighlightSample()
{
	ProcessedSampleSelector dlg(this, false);
	if (dlg.exec()==QDialog::Accepted && dlg.isValidSelection())
	{
		addHighlightedProcessedSampleById(dlg.processedSampleId(), dlg.processedSampleName());
	}
}

void DBQCWidget::addHighlightRun()
{
	//create
	DBSelector* selector = new DBSelector(this);
	selector->fill(db_.createTable("sequencing_run", "SELECT id, name FROM sequencing_run"), true);

	//execute
	QSharedPointer<QDialog> dialog = GUIHelper::createDialog(selector, "Select sequencing run", "Sequencing run:", true);
	if (dialog->exec()==QDialog::Accepted && selector->isValidSelection())
	{
		SqlQuery query = db_.getQuery();
		query.exec("SELECT ps.id, CONCAT(s.name,'_',LPAD(ps.process_id,2,'0')) FROM processed_sample ps, sample s WHERE ps.sample_id=s.id AND ps.sequencing_run_id=" + selector->getId());
		while (query.next())
		{
			addHighlightedProcessedSampleById(query.value(0).toString(), query.value(1).toString(), false);
		}
	}

	//update plot
	updatePlot();
}

void DBQCWidget::addHighlightProject()
{
	//create
	DBSelector* selector = new DBSelector(this);
	selector->fill(db_.createTable("project", "SELECT id, name FROM project"), true);

	//execute
	QSharedPointer<QDialog> dialog = GUIHelper::createDialog(selector, "Select project", "Project:", true);
	if (dialog->exec()==QDialog::Accepted && selector->isValidSelection())
	{
		QString query_string = "SELECT ps.id, CONCAT(s.name,'_',LPAD(ps.process_id,2,'0')) FROM processed_sample ps, sample s WHERE ps.sample_id=s.id AND ps.project_id=" + selector->getId();
		QString sys_id = ui_.system->getCurrentId();
		if (!sys_id.isEmpty())
		{
			query_string += " AND ps.processing_system_id='" + sys_id + "'";
		}
		SqlQuery query = db_.getQuery();
		query.exec(query_string);
		while (query.next())
		{
			addHighlightedProcessedSampleById(query.value(0).toString(), query.value(1).toString(), false);
		}
	}

	//update plot
	updatePlot();
}

void DBQCWidget::copyQcMetricsToClipboard()
{
	try
	{
		QApplication::setOverrideCursor(Qt::BusyCursor);

		//checks
		if(ui_.term->currentText().isEmpty()) THROW(ArgumentException, "QC metric 1 has to be set for export!");
		if (ui_.system->getCurrentId().isEmpty()) THROW(ArgumentException, "A processing system has to be set for export!");

		//create output
		QStringList output;
		output << "#" + ui_.term->currentText() + "\tsample\tsample_quality\trun\trun_date\tproject\tproject_type";
		QString term_id = ui_.term->getCurrentId();
		QString sys_id = ui_.system->getCurrentId();
		SqlQuery query = db_.getQuery();
		query.exec("SELECT qc.value, CONCAT(s.name,'_',LPAD(ps.process_id,2,'0')), ps.quality, r.name, r.end_date, p.name, p.type FROM processed_sample_qc qc, processed_sample ps, sequencing_run r, project p, sample s WHERE ps.sample_id=s.id AND ps.project_id=p.id AND qc.processed_sample_id=ps.id AND ps.sequencing_run_id=r.id AND qc.qc_terms_id='" + term_id + "' AND ps.processing_system_id='" + sys_id + "' ORDER BY ps.id ASC");
		while(query.next())
		{
			output << query.value(0).toString() + "\t" + query.value(1).toString() + "\t" + query.value(2).toString() + "\t" + query.value(3).toString() + "\t" + query.value(4).toString() + "\t" + query.value(5).toString() + "\t" + query.value(6).toString();
		}

		//output
		QApplication::clipboard()->setText(output.join("\n"));
		QApplication::restoreOverrideCursor();
		QMessageBox::information(this, "QC export", "Copied " + QString::number(query.size()) + " QC values to clipboard.");
	}
	catch(Exception& e)
	{
		QApplication::restoreOverrideCursor();
		QMessageBox::warning(this, "QC export", "Error: " + e.message());
	}
}

void DBQCWidget::checkHighlightedSamples()
{
	highlight_.clear();

	QStringList invalid;
	QStringList parts = ui_.highlight->text().replace("\t", " ").split(" ");
	foreach(QString part, parts)
	{
		part = part.trimmed();
		if (part.isEmpty()) continue;

		QString id = db_.processedSampleId(part, false);
		if (id.isEmpty())
		{
			invalid  << part;
			continue;
		}

		highlight_ << id;
	}

	if(invalid.count()>0)
	{
		ui_.highlight->setStyleSheet("QLineEdit {border: 2px solid red;}");
		ui_.highlight->setToolTip("Invalid processed sample names:\n" + invalid.join("\n"));
	}
	else
	{
		ui_.highlight->setStyleSheet("QLineEdit {}");
		ui_.highlight->setToolTip("");
	}

	updatePlot();
}

void DBQCWidget::setTermId(QString id)
{
	ui_.term->setCurrentId(id);
}

void DBQCWidget::setSecondTermId(QString id)
{
	ui_.term2->setCurrentId(id);
}

void DBQCWidget::updatePlot()
{
	//clear (necessary because settings the chart, releases the ownership of the previous chart)
	QChart* chart_old = ui_.plot->chart();
	ui_.plot->setChart(new QChart());
	delete chart_old;

	//check that at least one quality metric is given
	QString term_id = ui_.term->getCurrentId();
	if (term_id.isEmpty())
	{
		return;
	}

	QApplication::setOverrideCursor(Qt::BusyCursor);

	//determine plot type
	bool scatterplot = !ui_.term2->currentText().isEmpty();

	QString sample_type_filter = "";
	if (ui_.sample_type->currentText() == "tumor only")
	{
		sample_type_filter = " AND s.tumor = 1";
	} else if (ui_.sample_type->currentText() == "normal only")
	{
		sample_type_filter = " AND s.tumor = 0";
	}

	//create query
	QString query_string;
	if (scatterplot)
	{
		QString term_id2 = ui_.term2->getCurrentId();
		query_string = "SELECT qc.value, ps.quality, qc2.value, ps.id FROM processed_sample_qc qc, processed_sample_qc qc2, processed_sample ps LEFT JOIN sample as s ON ps.sample_id = s.id WHERE qc.processed_sample_id=ps.id AND qc2.processed_sample_id=ps.id AND qc.qc_terms_id='" + term_id + "' AND qc2.qc_terms_id='" + term_id2 + "'" + sample_type_filter;
	}
	else
	{
		query_string = "SELECT qc.value, ps.quality, r.end_date, ps.id FROM processed_sample_qc qc, processed_sample ps LEFT JOIN sample as s ON ps.sample_id = s.id, sequencing_run r WHERE qc.processed_sample_id=ps.id AND ps.sequencing_run_id=r.id AND qc.qc_terms_id='" + term_id + "'" + sample_type_filter;
	}
	QString sys_id = ui_.system->getCurrentId();
	if (!sys_id.isEmpty())
	{
		query_string += " AND ps.processing_system_id='" + sys_id + "'";
	}

	//execute query
	SqlQuery query = db_.getQuery();
	query.exec(query_string);

	//perform statistics
	ui_.count->setText("n/a");
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
	series.insert("good", getSeries("qc: good", QColor("#11A50F")));
	series.insert("medium", getSeries("qc: medium", QColor("#FF9600")));
	series.insert("bad", getSeries("qc: bad", QColor("#FF0000")));
	series.insert("n/a", getSeries("qc: n/a", Qt::gray));
	series.insert("highlight", getSeries("highlight", Qt::black, true));
	double value_min = std::numeric_limits<double>::max();
	double value_max = -std::numeric_limits<double>::max();
	double value2_min = std::numeric_limits<double>::max();
	double value2_max = -std::numeric_limits<double>::max();
	query.seek(-1);
	while(query.next())
	{
		bool ok;
		double value = query.value(0).toDouble(&ok);
		if (!ok) continue; //value not a number
		if (value<value_min) value_min = value;
		if (value>value_max) value_max = value;

		double value2;
		if (scatterplot)
		{
			value2 = query.value(2).toDouble(&ok);
			if (!ok) continue; //value not a number
		}
		else
		{
			QDateTime datetime = query.value(2).toDateTime();
			if (!datetime.isValid()) continue; //date can be null
			value2 = datetime.toMSecsSinceEpoch();
		}
		if (value2<value2_min) value2_min = value2;
		if (value2>value2_max) value2_max = value2;

		QString quality = query.value(1).toString();
		QString ps_id = query.value(3).toString();
		if(highlight_.contains(ps_id))
		{
			quality = "highlight";
		}
		if (!series.contains(quality))
		{
			THROW(ProgrammingException, "Invalid processed sample quality '" + quality + "' in DBQCWidget::updateGUI!");
		}


		series[quality]->append(value2, value);
	}

	//create chart
	QChart* chart = new QChart();
	chart->legend()->setVisible(true);
	chart->legend()->setAlignment(Qt::AlignBottom);

	//add axes
	QAbstractAxis* x_axis;
	if (scatterplot)
	{
		QValueAxis* axis = new QValueAxis();
		axis->setTitleText(ui_.term2->currentText());
		axis->setTickCount(8);
		x_axis = axis;
	}
	else
	{
		QDateTimeAxis* axis = new QDateTimeAxis();
		axis->setFormat("dd.MM.yyyy");
		axis->setTitleText("Date");
		x_axis = axis;
	}
	chart->addAxis(x_axis, Qt::AlignBottom);

	QValueAxis* y_axis = new QValueAxis();
	y_axis->setTitleText(ui_.term->currentText());
	y_axis->setTickCount(8);
	chart->addAxis(y_axis, Qt::AlignLeft);

	//add data series
	addSeries(chart, x_axis, y_axis, series["n/a"]);
	addSeries(chart, x_axis, y_axis, series["good"]);
	addSeries(chart, x_axis, y_axis, series["medium"]);
	addSeries(chart, x_axis, y_axis, series["bad"]);
	addSeries(chart, x_axis, y_axis, series["highlight"]);


	//margin X (prevent clipping of data point icons)
	double x_margin = 0.01*(value2_max==value2_min ? value2_max : value2_max-value2_min);
	if (x_margin>0)
	{
		if (scatterplot)
		{
			x_axis->setMin(value2_min-x_margin);
			x_axis->setMax(value2_max+x_margin);
		}
		else
		{
			x_axis->setMin(QDateTime::fromMSecsSinceEpoch(value2_min-x_margin));
			x_axis->setMax(QDateTime::fromMSecsSinceEpoch(value2_max+x_margin));
		}
	}

	//margin Y (prevent clipping of data point icons)
	double y_margin = 0.01*(value_max==value_min ? value_max : value_max-value_min);
	if (y_margin>0)
	{
		y_axis->setMin(value_min-y_margin);
		y_axis->setMax(value_max+y_margin);
	}

	//update GUI
	chart->legend()->setMarkerShape(QLegend::MarkerShapeFromSeries);
	ui_.plot->setRenderHint(QPainter::Antialiasing);
	ui_.plot->setChart(chart);

	QApplication::restoreOverrideCursor();
}

void DBQCWidget::resetZoom()
{
	QChart* chart = ui_.plot->chart();
	if (chart==nullptr) return;

	chart->zoomReset();
}

void DBQCWidget::swapMetrics()
{
	//check that two metrics are given
	QString term1 = ui_.term->currentText();
	QString term2 = ui_.term2->currentText();
	if (term1.isEmpty() || term2.isEmpty()) return;

	//update - avoid updating the GUI twice by blocking the first signal
	ui_.term->blockSignals(true);
	ui_.term->setCurrentText(term2);
	ui_.term->blockSignals(false);
	ui_.term2->setCurrentText(term1);
}

QScatterSeries* DBQCWidget::getSeries(QString name, QColor color, bool square)
{
	QScatterSeries* series = new QScatterSeries();

	series->setMarkerShape(square ? QScatterSeries::MarkerShapeRectangle : QScatterSeries::MarkerShapeCircle);
	series->setBorderColor(Qt::transparent);
	series->setBrush(color);
	series->setOpacity(0.7);
	series->setMarkerSize(6);
	series->setName(name);

	return series;
}

void DBQCWidget::addSeries(QChart* chart, QAbstractAxis* x_axis, QAbstractAxis* y_axis, QAbstractSeries* series)
{
	chart->addSeries(series);
	series->attachAxis(x_axis);
	series->attachAxis(y_axis);
}
