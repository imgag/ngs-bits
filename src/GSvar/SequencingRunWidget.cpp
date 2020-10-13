#include "SequencingRunWidget.h"
#include "ui_SequencingRunWidget.h"
#include "ProcessedSampleWidget.h"
#include "MidCheckWidget.h"
#include "DBEditor.h"
#include "GUIHelper.h"
#include "Settings.h"
#include "MidCheck.h"
#include "LoginManager.h"
#include "EmailDialog.h"
#include "DBQCWidget.h"
#include <QMessageBox>
#include <QInputDialog>

SequencingRunWidget::SequencingRunWidget(QWidget* parent, QString run_id)
	: QWidget(parent)
	, ui_(new Ui::SequencingRunWidget)
	, run_id_(run_id)
{
	ui_->setupUi(this);
	GUIHelper::styleSplitter(ui_->splitter);
	ui_->splitter->setSizes(QList<int>() << 200 << 800);
	connect(ui_->show_qc, SIGNAL(toggled(bool)), this, SLOT(updateGUI()));
	connect(ui_->update_btn, SIGNAL(clicked(bool)), this, SLOT(updateGUI()));
	connect(ui_->edit_btn, SIGNAL(clicked(bool)), this, SLOT(edit()));
	connect(ui_->email_btn, SIGNAL(clicked(bool)), this, SLOT(sendStatusEmail()));
	connect(ui_->mid_check_btn, SIGNAL(clicked(bool)), this, SLOT(checkMids()));
	connect(ui_->samples, SIGNAL(rowDoubleClicked(int)), this, SLOT(openSampleTab(int)));
	QAction* action = new QAction(QIcon(":/Icons/NGSD_sample.png"), "Open processed sample tab", this);
	ui_->samples->addAction(action);
	connect(action, SIGNAL(triggered(bool)), this, SLOT(openSelectedSampleTabs()));

	//set quality
	action = new QAction("Set quality", this);
	ui_->samples->addAction(action);
	connect(action, SIGNAL(triggered(bool)), this, SLOT(setQuality()));

	//QC plot
	ui_->samples->setSelectionBehavior(QAbstractItemView::SelectItems);
	action = new QAction("Plot", this);
	ui_->samples->addAction(action);
	connect(action, SIGNAL(triggered(bool)), this, SLOT(showPlot()));

	updateGUI();
}

SequencingRunWidget::~SequencingRunWidget()
{
	delete ui_;
}

void SequencingRunWidget::updateGUI()
{
	try
	{
		//#### run details ####
		NGSD db;
		SqlQuery query = db.getQuery();
		query.exec("SELECT r.*, d.name d_name, d.type d_type FROM sequencing_run r, device d WHERE r.device_id=d.id AND r.id='" + run_id_ + "'");
		query.next();
		ui_->name->setText(query.value("name").toString());
		ui_->fcid->setText(query.value("fcid").toString());
		ui_->fc_type->setText(query.value("flowcell_type").toString());
		ui_->start->setText(query.value("start_date").toString());
		ui_->end->setText(query.value("end_date").toString());
		ui_->recipe->setText(query.value("recipe").toString());
		ui_->comments->setText(query.value("comment").toString());
		ui_->device->setText(query.value("d_name").toString() + " (" + query.value("d_type").toString() + ")");
		QVariant molarity = query.value("pool_molarity");
		if (!molarity.isNull())
		{
			ui_->molarity_and_method->setText(molarity.toString() + " (" + query.value("pool_quantification_method").toString() + ")");
		}
		else
		{
			ui_->molarity_and_method->setText("");
		}
		ProcessedSampleWidget::styleQualityLabel(ui_->quality, query.value("quality").toString());
		QString status = query.value("status").toString();
		ui_->status->setText(status);
		ui_->backup->setText(query.value("backup_done").toString()=="1" ? "yes" : "no");

		//#### run quality ####
		updateReadQualityTable();

		//#### sample table ####
		updateRunSampleTable();
	}
	catch (Exception& e)
	{
		QMessageBox::warning(this, "Update failed", "Could not update data:\n" + e.message());
	}

}

void SequencingRunWidget::updateRunSampleTable()
{
	//get data from NGSD
	QStringList headers;
	headers << "lane" << "quality" << "sample" << "name external" << "is_tumor" << "is_ffpe" << "project" << "MID i7" << "MID i5" << "species" << "processing system" << "input [ng]" << "operator" << "comments";

	NGSD db;
	DBTable samples = db.createTable("processed_sample", "SELECT ps.id, ps.lane, ps.quality, CONCAT(s.name,'_',LPAD(ps.process_id,2,'0')), s.name_external, s.tumor, s.ffpe, (SELECT CONCAT(name, ' (', type, ')') FROM project WHERE id=ps.project_id), (SELECT CONCAT(name, ' (', sequence, ')') FROM mid WHERE id=ps.mid1_i7), (SELECT CONCAT(name, ' (', sequence, ')') FROM mid WHERE id=ps.mid2_i5), (SELECT name FROM species WHERE id=s.species_id), (SELECT name_manufacturer FROM processing_system WHERE id=ps.processing_system_id), ps.processing_input, (SELECT name FROM user WHERE id=ps.operator_id), ps.comment "
														  " FROM processed_sample ps, sample s WHERE ps.sample_id=s.id AND ps.sequencing_run_id='" + run_id_ + "' "
														  " ORDER BY ps.lane ASC, s.name ASC, ps.process_id");

	//format columns
	samples.formatBooleanColumn(samples.columnIndex("tumor"));
	samples.formatBooleanColumn(samples.columnIndex("ffpe"));

	//add QC data
	QStringList accessions;
	accessions << "QC:2000005" << "QC:2000023" << "QC:2000021" << "QC:2000024" << "QC:2000025" << "QC:2000027" << "QC:2000013" << "QC:2000014" << "QC:2000051";
	if (ui_->show_qc->isChecked())
	{
		//create column data
		QList<QStringList> cols;
		while(cols.count()< accessions.count()) cols << QStringList();
		for (int r=0; r<samples.rowCount(); ++r)
		{
			QCCollection qc_data = db.getQCData(samples.row(r).id());
			for(int i=0; i<accessions.count(); ++i)
			{
				try
				{
					QString value = qc_data.value(accessions[i], true).toString();
					cols[i] << value;
				}
				catch(...)
				{
					cols[i] << "";
				}
			}
		}
		//add columns
		for(int i=0; i<accessions.count(); ++i)
		{
			QString header = db.getValue("SELECT name FROM qc_terms WHERE qcml_id=:0", true, accessions[i]).toString();
			header.replace("percentage", "%");
			samples.addColumn(cols[i], header);
			headers << header;

			//add 'read %' column
			if (accessions[i]=="QC:2000005")
			{
				double sum = 0;
				foreach(const QString& count, cols[i])
				{
					if (!count.isEmpty()) sum += count.toDouble();
				}
				QStringList col;
				foreach(const QString& count, cols[i])
				{
					if (count.isEmpty())
					{
						col << "";
					}
					else
					{
						col << QString::number(100.0 * count.toDouble() / sum, 'f', 2);
					}
				}
				samples.addColumn(col, "read %");
				headers << "read %";
			}
		}
	}
	samples.setHeaders(headers);

	//show table in GUI
	QStringList quality_values = samples.takeColumn(samples.columnIndex("quality"));
	ui_->samples->setData(samples);
	ui_->samples->setQualityIcons("sample", quality_values);
	ui_->samples->setColumnWidth(ui_->samples->columnIndex("comments"), 350);

	//colors
	QColor orange = QColor(255,150,0,125);
	QColor red = QColor(255,0,0,125);
	if (ui_->show_qc->isChecked())
	{
		for(int i=0; i<accessions.count(); ++i)
		{
			QString accession = accessions[i];
			QString header = headers[headers.count()-accessions.count()+i];

			if (accession=="QC:2000014") //known variants %
			{
				ui_->samples->setBackgroundColorIfLt(header, orange, 95);
				ui_->samples->setBackgroundColorIfLt(header, red, 90);
			}
			if (accession=="QC:2000023") //insert size
			{
				ui_->samples->setBackgroundColorIfLt(header, orange, 190);
				ui_->samples->setBackgroundColorIfLt(header, red, 150);
			}
			if (accession=="QC:2000025") //avg depth
			{
				ui_->samples->setBackgroundColorIfLt(header, orange, 80);
				ui_->samples->setBackgroundColorIfLt(header, red, 30);
			}
			if (accession=="QC:2000027") //cov 20x
			{
				ui_->samples->setBackgroundColorIfLt(header, orange, 95);
				ui_->samples->setBackgroundColorIfLt(header, red, 90);
			}
			if (accession=="QC:2000024") //duplicates
			{
				ui_->samples->setBackgroundColorIfGt(header, orange, 25);
				ui_->samples->setBackgroundColorIfGt(header, red, 35);
			}
			if (accession=="QC:2000051") //AF deviation
			{
				ui_->samples->setBackgroundColorIfGt(header, orange, 3);
				ui_->samples->setBackgroundColorIfGt(header, red, 6);
			}
			if (accession=="QC:2000021") //on target
			{
				ui_->samples->setBackgroundColorIfLt(header, orange, 50);
				ui_->samples->setBackgroundColorIfLt(header, red, 25);
			}
		}
	}
	ui_->samples->setBackgroundColorIfEqual("is_tumor", orange, "yes");
	ui_->samples->setBackgroundColorIfEqual("is_ffpe", orange, "yes");

	//#### sample summary ####
	QStringList imported_qc = db.getValues("SELECT ps.id FROM processed_sample ps WHERE ps.sequencing_run_id='" + run_id_ + "' AND EXISTS(SELECT id FROM processed_sample_qc WHERE processed_sample_id=ps.id)");
	QStringList imported_vars = db.getValues("SELECT ps.id FROM processed_sample ps WHERE ps.sequencing_run_id='" + run_id_ + "' AND EXISTS(SELECT variant_id FROM detected_variant WHERE processed_sample_id=ps.id)");
	ui_->sample_count->setText(QString::number(samples.rowCount()) + " samples (" + QString::number(imported_qc.count()) + " with QC, " + QString::number(imported_vars.count()) + " with variants)");
}

void SequencingRunWidget::setQuality()
{
	NGSD db;
	QStringList qualities = db.getEnum("processed_sample", "quality");

	//get quality from user
	bool ok;
	QString quality = QInputDialog::getItem(this, "Select processed sample quality", "quality:", qualities, 0, false, &ok);
	if (!ok) return;

	//prepare query
	SqlQuery query = db.getQuery();
	query.prepare("UPDATE processed_sample SET quality='" + quality + "' WHERE id=:0");

	int col = ui_->samples->columnIndex("sample");
	QList<int> selected_rows = ui_->samples->selectedRows().toList();
	foreach (int row, selected_rows)
	{
		QString ps_name = ui_->samples->item(row, col)->text();
		QString ps_id = db.processedSampleId(ps_name);
		query.bindValue(0, ps_id);
		query.exec();
	}

	updateGUI();
}

void SequencingRunWidget::showPlot()
{
	NGSD db;

	//check one QC cell is selected
	QList<int> selected_cols = ui_->samples->selectedColumns().toList();
	QList<int> selected_rows = ui_->samples->selectedRows().toList();
	if (selected_cols.count()!=1 || selected_rows.count()!=1)
	{
		QMessageBox::information(this, "Plot error", "Please select <b>exactly one cell</b> containing a <b>quality metric</b> for plotting!");
		return;
	}
	int col = selected_cols[0];
	QString qc_term_id = db.getValue("SELECT id FROM qc_terms WHERE name='" + ui_->samples->columnHeader(col).replace("%", "percentage") + "'", true).toString();
	if (qc_term_id.isEmpty())
	{
		QMessageBox::information(this, "Plot error", "Please select <b>exactly one cell</b> containing a <b>quality metric</b> for plotting!");
		return;
	}

	//create widget
	DBQCWidget* qc_widget = new DBQCWidget(this);
	//highlight all samples on this run
	SqlQuery query = db.getQuery();
	query.exec("SELECT ps.id, CONCAT(s.name,'_',LPAD(ps.process_id,2,'0')) FROM processed_sample ps, sample s WHERE ps.sample_id=s.id AND ps.sequencing_run_id=" + run_id_);
	while (query.next())
	{
		qc_widget->addHighlightedProcessedSampleById(query.value(0).toString(), query.value(1).toString(), false);
	}

	//show widget
	int row = selected_rows[0];
	qc_widget->setSystemId(db.getValue("SELECT processing_system_id FROM processed_sample WHERE id='" + ui_->samples->getId(row) + "'", true).toString());
	qc_widget->setTermId(qc_term_id);
	auto dlg = GUIHelper::createDialog(qc_widget, "QC plot");
	dlg->exec();
}

void SequencingRunWidget::edit()
{
	DBEditor* widget = new DBEditor(this, "sequencing_run", run_id_.toInt());
	auto dlg = GUIHelper::createDialog(widget, "Edit sequencing run " + ui_->name->text() ,"", true);
	if (dlg->exec()==QDialog::Accepted)
	{
		widget->store();
		updateGUI();
	}
}

void SequencingRunWidget::sendStatusEmail()
{
	QString status = ui_->status->text();
	if (status!="analysis_finished" && status!="run_finished")
	{
		QMessageBox::information(this, "Run status email", "Run status emails can only be generated for runs with status 'analysis_finished' or 'run_finished'!");
		return;
	}

	NGSD db;
	QString run_name = ui_->name->text();

	//create email
	QStringList to, body;
	QString subject;

	if (status=="analysis_finished")
	{
		to << Settings::string("email_run_analyzed").split(";");

		subject = "[NGSD] Lauf " + run_name + " analysiert";

		body << "Hallo zusammen,";
		body << "";
		body << "der Lauf " + run_name + " ist fertig analysiert:";

		SqlQuery query = db.getQuery();
		query.exec("SELECT DISTINCT p.id, p.name, p.email_notification, p.internal_coordinator_id, p.analysis FROM project p, processed_sample ps WHERE ps.project_id=p.id AND ps.sequencing_run_id=" + run_id_ + " ORDER BY p.name ASC");
		while(query.next())
		{
			int coordinator_id = query.value("internal_coordinator_id").toInt();
			to << query.value("email_notification").toString().split(";");
			to << db.userEmail(coordinator_id);

			body << "";
			body << "Projekt: " + query.value("name").toString();
			body << "  Koordinator: " + db.userName(coordinator_id);
			QStringList operator_ids = db.getValues("SELECT operator_id FROM processed_sample WHERE sequencing_run_id='" + run_id_ + "' AND project_id='" + query.value("id").toString() + "' AND operator_id IS NOT NULL");
			body << "  Proben: " + QString::number(operator_ids.count());
			body << "  Analyse: " + query.value("analysis").toString();

			operator_ids.removeDuplicates();
			operator_ids.removeAll("");
			foreach(QString operator_id, operator_ids)
			{
				to << db.userEmail(operator_id.toInt());
			}
		}
	}
	else if (status=="run_finished")
	{
		to << Settings::string("email_run_finished").split(";");

		subject = "[NGSD] Lauf " + run_name + " ist sequenziert";

		body << "Hallo zusammen,";
		body << "";
		body << "der Lauf " + run_name + " ist fertig sequenziert.";
	}

	body << "";
	body << "Viele Gruesse, ";
	body << "  " + LoginManager::userName();

	//send
	EmailDialog dlg(this, to, subject, body);
	dlg.exec();
}

void SequencingRunWidget::checkMids()
{
	try
	{
		NGSD db;

		//create dialog
		MidCheckWidget* widget = new MidCheckWidget();
		widget->addRun(db.getValue("SELECT name FROM sequencing_run WHERE id=" + run_id_).toString());

		//determine usable length
		QString recipe = db.getValue("SELECT recipe FROM sequencing_run WHERE id=" + run_id_).toString();
		QPair<int,int> lengths = MidCheck::lengthFromRecipe(recipe);
		QPair<int,int> tmp = MidCheck::lengthFromSamples(widget->mids());
		lengths.first = std::min(lengths.first, tmp.first);
		lengths.second = std::min(lengths.second, tmp.second);
		widget->setParameters(lengths);

		//show dialog
		auto dlg = GUIHelper::createDialog(widget, "MID clash detection - sequencing run " + ui_->name->text());
		dlg->exec();
	}
	catch (Exception& e)
	{
		QMessageBox::warning(this, "MID clash detection", "Error: MID clash detection could not be performed:\n" + e.message());
	}
}

void SequencingRunWidget::highlightItem(QTableWidgetItem* item)
{
	QFont font = item->font();
	font.setBold(true);
	item->setFont(font);
}

void SequencingRunWidget::updateReadQualityTable()
{
	QTableWidget* table = ui_->read_quality;

	//clear everything
	table->clearContents();
	table->setRowCount(0);

	//set header
	QStringList headers;
	headers << "read" << "lane" << "Q30 [%]" << "error rate [%]" << "clusters [K/mm2]" << "clusters PF [%]" << "yield [GB]";

	table->setColumnCount(headers.count());
	for(int c=0; c<headers.count(); ++c)
	{
		table->setHorizontalHeaderItem(c, GUIHelper::createTableItem(headers[c], Qt::AlignCenter));
	}
	NGSD db;
	SqlQuery q_reads = db.getQuery();
	q_reads.exec("SELECT * FROM runqc_read WHERE sequencing_run_id=" + run_id_ + " ORDER BY read_num ASC");
	while(q_reads.next())
	{
		//all lanes summary line
		int row = table->rowCount();
		table->setRowCount(row + 1);

		QString text = "R" + q_reads.value("read_num").toString() + " (" + q_reads.value("cycles").toString() + "cycles";
		if (q_reads.value("is_index").toInt()==1) text += ", index";
		text += ")";
		QTableWidgetItem* item = GUIHelper::createTableItem(text);
		highlightItem(item);
		table->setItem(row, 0, item);

		item = GUIHelper::createTableItem("all");
		highlightItem(item);
		table->setItem(row, 1, item);

		item = GUIHelper::createTableItem(q_reads.value("q30_perc").toString());
		highlightItem(item);
		table->setItem(row, 2, item);

		item = GUIHelper::createTableItem(q_reads.value("error_rate").toString());
		highlightItem(item);
		table->setItem(row, 3, item);

		//inidividual lanes
		SqlQuery q_lanes = db.getQuery();
		q_lanes.exec("SELECT * FROM runqc_lane WHERE runqc_read_id=" + q_reads.value("id").toString() + " ORDER BY lane_num ASC");
		while(q_lanes.next())
		{

			row = table->rowCount();
			table->setRowCount(row + 1);

			table->setItem(row, 0, GUIHelper::createTableItem(""));
			table->setItem(row, 1, GUIHelper::createTableItem(q_lanes.value("lane_num").toString()));
			table->setItem(row, 2, GUIHelper::createTableItem(QString::number(q_lanes.value("q30_perc").toDouble(), 'f', 2)));
			table->setItem(row, 3, GUIHelper::createTableItem(QString::number(q_lanes.value("error_rate").toDouble(), 'f', 2)));
			double density = q_lanes.value("cluster_density").toDouble();
			table->setItem(row, 4, GUIHelper::createTableItem(QString::number(density/1000.0, 'f', 2)));
			table->setItem(row, 5, GUIHelper::createTableItem(QString::number(q_lanes.value("cluster_density_pf").toDouble()/density*100.0, 'f', 2)));
			table->setItem(row, 6, GUIHelper::createTableItem(QString::number(q_lanes.value("yield").toDouble()/1000000000.0, 'f', 2)));
		}
	}
	GUIHelper::resizeTableCells(table);
}

void SequencingRunWidget::openSelectedSampleTabs()
{
	int col = ui_->samples->columnIndex("sample");
	QList<int> selected_rows = ui_->samples->selectedRows().toList();
	foreach (int row, selected_rows)
	{
		QTableWidgetItem* item = ui_->samples->item(row, col);

		emit(openProcessedSampleTab(item->text()));
	}
}

void SequencingRunWidget::openSampleTab(int row)
{
	int col = ui_->samples->columnIndex("sample");
	emit(openProcessedSampleTab(ui_->samples->item(row, col)->text()));
}
