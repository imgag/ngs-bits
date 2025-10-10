#include "DBQCWidget.h"
#include "BasicStatistics.h"
#include "DBSelector.h"
#include "GUIHelper.h"
#include "ProcessedSampleSelector.h"

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

	//QC terms
	ui_.term->fill(db_.createTable("qc_terms", "SELECT id, name FROM qc_terms WHERE obsolete=0 ORDER BY qcml_id ASC"));
	connect(ui_.term, SIGNAL(currentTextChanged(QString)), this, SLOT(updatePlot()));
	ui_.term2->fill(db_.createTable("qc_terms", "SELECT id, name FROM qc_terms WHERE obsolete=0 ORDER BY qcml_id ASC"));
	connect(ui_.term2, SIGNAL(currentTextChanged(QString)), this, SLOT(updatePlot()));

	//processing system
	ui_.processing_system->fill(db_.createTable("processing_system", "SELECT id, name_manufacturer FROM processing_system ORDER BY name_manufacturer ASC"));
	connect(ui_.processing_system, SIGNAL(currentTextChanged(QString)), this, SLOT(updatePlot()));
	ui_.processing_modus->addItem("");
	ui_.processing_modus->addItems(db_.getEnum("processed_sample", "processing_modus"));
	connect(ui_.processing_modus, SIGNAL(currentTextChanged(QString)), this, SLOT(updatePlot()));
	connect(ui_.processing_input, SIGNAL(editingFinished()), this, SLOT(updatePlot()));

	//sample
	connect(ui_.sample_type, SIGNAL(currentTextChanged(QString)), this, SLOT(updatePlot()));
	ui_.sample_tissue->addItem("");
	ui_.sample_tissue->addItems(db_.getEnum("sample", "tissue"));
	connect(ui_.sample_tissue, SIGNAL(currentTextChanged(QString)), this, SLOT(updatePlot()));

	//sequencing run
	ui_.run_device->fill(db_.createTable("device", "SELECT id, CONCAT(name, ' (', type, ')') FROM device ORDER BY id DESC"));
	connect(ui_.run_device, SIGNAL(currentTextChanged(QString)), this, SLOT(updatePlot()));
	ui_.run_side->addItem("");
	ui_.run_side->addItems(db_.getEnum("sequencing_run", "side"));
	connect(ui_.run_side, SIGNAL(currentTextChanged(QString)), this, SLOT(updatePlot()));
	connect(ui_.run_after, SIGNAL(editingFinished()), this, SLOT(updatePlot()));
	connect(ui_.run_before, SIGNAL(editingFinished()), this, SLOT(updatePlot()));

	//project
	ui_.project_type->addItem("");
	ui_.project_type->addItems(db_.getEnum("project", "type"));
	connect(ui_.project_type, SIGNAL(currentTextChanged(QString)), this, SLOT(updatePlot()));

	//misc
	connect(ui_.highlight, SIGNAL(editingFinished()), this, SLOT(checkHighlightedSamples()));
	connect(ui_.export_btn, SIGNAL(clicked(bool)), this, SLOT(copyQcMetricsToClipboard()));
}

void DBQCWidget::setSystemId(QString id)
{
	ui_.processing_system->setCurrentId(id);
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
		QString sys_id = ui_.processing_system->getCurrentId();
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
		if (ui_.processing_system->getCurrentId().isEmpty()) THROW(ArgumentException, "A processing system has to be set for export!");

		//create output
		QStringList output;
		output << "#" + ui_.term->currentText() + "\tsample\tsample_quality\trun\trun_date\tproject\tproject_type";
		QString term_id = ui_.term->getCurrentId();
		QString sys_id = ui_.processing_system->getCurrentId();
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
	if (term_id.isEmpty()) return;

	try
	{
		QApplication::setOverrideCursor(Qt::BusyCursor);

		//determine plot type
		bool scatterplot = !ui_.term2->currentText().isEmpty();

		//create query
		QString query_string;
		if (scatterplot)
		{
			QString term_id2 = ui_.term2->getCurrentId();
			query_string = "SELECT qc.value, ps.quality, qc2.value, ps.id FROM processed_sample_qc qc, processed_sample_qc qc2, processed_sample ps, sample s, sequencing_run r, processing_system sys, project p WHERE ps.project_id=p.id AND ps.sequencing_run_id=r.id AND ps.sample_id=s.id AND ps.sequencing_run_id=r.id AND ps.processing_system_id=sys.id AND qc.processed_sample_id=ps.id AND qc2.processed_sample_id=ps.id AND qc.qc_terms_id='" + term_id + "' AND qc2.qc_terms_id='" + term_id2 + "'";
		}
		else
		{
			query_string = "SELECT qc.value, ps.quality, r.end_date, ps.id FROM processed_sample_qc qc, processed_sample ps, sample s, sequencing_run r, processing_system sys, project p WHERE ps.sample_id=s.id AND ps.project_id=p.id AND ps.sequencing_run_id=r.id AND qc.processed_sample_id=ps.id AND ps.sequencing_run_id=r.id AND ps.processing_system_id=sys.id AND qc.qc_terms_id='" + term_id + "'";
		}

		//add sample constraints to query
		QString sample_type = ui_.sample_type->currentText().trimmed();
		if (sample_type=="tumor (FFPE)")
		{
			query_string += " AND s.tumor = 1 AND s.ffpe = 1";
		}
		else if (sample_type=="tumor (no FFPE)")
		{
			query_string += " AND s.tumor = 1 AND s.ffpe = 0";
		}
		else if (sample_type=="normal (no FFPE)")
		{
			query_string += " AND s.tumor = 0 AND s.ffpe = 0";
		}
		else if (sample_type=="normal (FFPE)")
		{
			query_string += " AND s.tumor = 0 AND s.ffpe = 1";
		}
		QString tissue = ui_.sample_tissue->currentText().trimmed();
		if (!tissue.isEmpty())
		{
			query_string += " AND s.tissue='" + tissue + "'";
		}

		//add processed sample constraints to query
		QString sys_id = ui_.processing_system->getCurrentId();
		if (!sys_id.isEmpty())
		{
			query_string += " AND sys.id='" + sys_id + "'";
		}
		QString modus = ui_.processing_modus->currentText().trimmed();
		if (!modus.isEmpty())
		{
			query_string += " AND ps.processing_modus='" + modus + "'";
		}
		QString input = ui_.processing_input->text().trimmed();
		if (!input.isEmpty())
		{
			bool ok = false;
			double input_value = input.toDouble(&ok);
			if (!ok) WARNING(ArgumentException, "Could not convert processing input '" + input + "' to double");
			query_string += " AND ps.processing_input>=" + QString::number(input_value-0.1) + " AND ps.processing_input<=" + QString::number(input_value+0.1);
		}

		//add run constraints to query
		QString device_id = ui_.run_device->getCurrentId();
		if (!device_id.isEmpty())
		{
			query_string += " AND r.device_id='" + device_id + "'";
		}
		QString side = ui_.run_side->currentText().trimmed();
		if (!side.isEmpty())
		{
			query_string += " AND r.side='" + side + "'";
		}
		QString run_after = ui_.run_after->text().trimmed();
		if(run_after!="")
		{
			QDate run_after_date = QDate::fromString(run_after, Qt::ISODate);
			if (!run_after_date.isValid()) WARNING(ArgumentException, "Invalid date format for run after given.\nThe expected format is a ISO date, e.g. '2012-09-27'.");
			query_string += " AND r.start_date>='" + run_after_date.toString(Qt::ISODate) + "'";
		}
		QString run_before = ui_.run_before->text().trimmed();
		if(run_before!="")
		{
			QDate run_before_date = QDate::fromString(run_before, Qt::ISODate);
			if (!run_before_date.isValid()) WARNING(ArgumentException, "Invalid date format for run before given.\nThe expected format is a ISO date, e.g. '2012-09-27'.");
			query_string += " AND r.end_date<='" + run_before_date.toString(Qt::ISODate) + "'";
		}

		//ad project constraints
		QString project_type = ui_.project_type->currentText().trimmed();
		if (!project_type.isEmpty())
		{
			query_string += " AND p.type='" + project_type + "'";
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

		//check if all dates are unset > use 01.01.2000
		bool all_dates_unset = true;
		if (!scatterplot)
		{
			query.seek(-1);
			while(query.next())
			{
				if (query.value(2).toDateTime().isValid())
				{
					all_dates_unset = false;
					break;
				}
			}
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
				if (!datetime.isValid()) //date can be null
				{
					if (!all_dates_unset) continue; //skip data points without date
                    datetime.setDate(QDate(2000, 1, 1));
				}
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
	catch(Exception& e)
	{
		GUIHelper::showException(this, e, "QC plot exception");
	}
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
