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
#include "GlobalServiceProvider.h"
#include "BasicStatistics.h"
#include "GSvarHelper.h"
#include <numeric>

SequencingRunWidget::SequencingRunWidget(QWidget* parent, QString run_id)
	: QWidget(parent)
	, ui_(new Ui::SequencingRunWidget)
	, run_id_(run_id)
{
	ui_->setupUi(this);
	GUIHelper::styleSplitter(ui_->splitter);
	ui_->splitter->setSizes(QList<int>() << 200 << 800);
	connect(ui_->show_qc_cols, SIGNAL(toggled(bool)), this, SLOT(updateGUI()));
	connect(ui_->show_lab_cols, SIGNAL(toggled(bool)), this, SLOT(updateGUI()));
	connect(ui_->sort_by_ps_id, SIGNAL(toggled(bool)), this, SLOT(updateGUI()));
	connect(ui_->show_sample_comment, SIGNAL(toggled(bool)), this, SLOT(updateGUI()));
	connect(ui_->update_btn, SIGNAL(clicked(bool)), this, SLOT(updateGUI()));
	connect(ui_->edit_btn, SIGNAL(clicked(bool)), this, SLOT(edit()));
	connect(ui_->email_btn, SIGNAL(clicked(bool)), this, SLOT(sendStatusEmail()));
	connect(ui_->mid_check_btn, SIGNAL(clicked(bool)), this, SLOT(checkMids()));
	connect(ui_->samples, SIGNAL(rowDoubleClicked(int)), this, SLOT(openSampleTab(int)));
	connect(ui_->novaseqx_samplesheet_btn, SIGNAL(clicked(bool)), this, SLOT(exportSampleSheet()));
	QAction* action = new QAction(QIcon(":/Icons/NGSD_sample.png"), "Open processed sample tab", this);
	ui_->samples->addAction(action);
	connect(action, SIGNAL(triggered(bool)), this, SLOT(openSelectedSampleTabs()));

	//set quality
	action = new QAction("Set quality", this);
	ui_->samples->addAction(action);
	connect(action, SIGNAL(triggered(bool)), this, SLOT(setQuality()));

	//schedule re-sequencing
	action = new QAction("Schedule sample(s) for resequencing", this);
	ui_->samples->addAction(action);
	connect(action, SIGNAL(triggered(bool)), this, SLOT(scheduleForResequencing()));

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
		GSvarHelper::limitLines(ui_->comments, query.value("comment").toString());
		ui_->device->setText(query.value("d_name").toString() + " (" + query.value("d_type").toString() + ")");
		ui_->side->setText(query.value("side").toString());
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

		//#### activate SampleSheet ####
		ui_->novaseqx_samplesheet_btn->setEnabled((query.value("d_type").toString() == "NovaSeqXPlus") || (query.value("d_type").toString() == "NovaSeqX"));

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
	headers << "lane" << "quality" << "sample" << "name external" << "is_tumor" << "is_ffpe" << "sample type" << "project" << "MID i7" << "MID i5" << "species" << "processing system" << "input [ng]" << "molarity [nM]" << "operator" << "processing modus" << "batch number" << "comments";
	if (ui_->show_sample_comment->isChecked())
	{
		headers << "sample comments";
	}
	headers << "resequencing";
	NGSD db;
	DBTable samples = db.createTable("processed_sample", "SELECT ps.id, ps.lane, ps.quality, CONCAT(s.name,'_',LPAD(ps.process_id,2,'0')), s.name_external, s.tumor, s.ffpe, s.sample_type, (SELECT CONCAT(name, ' (', type, ')') FROM project WHERE id=ps.project_id), (SELECT CONCAT(name, ' (', sequence, ')') FROM mid WHERE id=ps.mid1_i7), (SELECT CONCAT(name, ' (', sequence, ')') FROM mid WHERE id=ps.mid2_i5), (SELECT name FROM species WHERE id=s.species_id), (SELECT name_manufacturer FROM processing_system WHERE id=ps.processing_system_id), ps.processing_input, ps.molarity, (SELECT name FROM user WHERE id=ps.operator_id), ps.processing_modus, ps.batch_number, ps.comment" + QString(ui_->show_sample_comment->isChecked() ? ", s.comment as sample_comment" : "") + " ,ps.scheduled_for_resequencing "+
														  " FROM processed_sample ps, sample s WHERE ps.sample_id=s.id AND ps.sequencing_run_id='" + run_id_ + "' "
														  " ORDER BY ps.lane ASC, "+ (ui_->sort_by_ps_id->isChecked() ? "ps.id" : "ps.processing_system_id ASC, s.name ASC, ps.process_id"));

	int count_not_wgs = db.getValue("SELECT count(ps.id) FROM processed_sample ps, processing_system sys WHERE sys.id=ps.processing_system_id AND ps.sequencing_run_id='"+run_id_+"' AND sys.type!='WGS'").toInt();
	bool is_wgs_run = count_not_wgs==0;

	//format columns
	samples.formatBooleanColumn(samples.columnIndex("tumor"));
	samples.formatBooleanColumn(samples.columnIndex("ffpe"));
	samples.formatBooleanColumn(samples.columnIndex("scheduled_for_resequencing"));

	// determine QC parameter based on sample types
	QSet<QString> sample_types = samples.extractColumn(samples.columnIndex("sample_type")).toSet();
	setQCMetricAccessions(sample_types);

	// update QC plot button
	ui_->plot_btn->setMenu(new QMenu());
	foreach(QString accession, qc_metric_accessions_)
	{
		QString name = db.getValue("SELECT name FROM qc_terms WHERE qcml_id=:0", true, accession).toString();
		name.replace("percentage", "%");
		ui_->plot_btn->menu()->addAction(name, this, SLOT(showPlot()))->setData(accession);
	}

	//add QC data
	const QStringList& accessions = qc_metric_accessions_;
	if (ui_->show_qc_cols->isChecked())
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

	//remove lab columns
	if (!ui_->show_lab_cols->isChecked())
	{
		samples.takeColumn(samples.columnIndex("MID i7"));
		samples.takeColumn(samples.columnIndex("MID i5"));
		samples.takeColumn(samples.columnIndex("input [ng]"));
		samples.takeColumn(samples.columnIndex("molarity [nM]"));
		samples.takeColumn(samples.columnIndex("operator"));
		samples.takeColumn(samples.columnIndex("processing modus"));
		samples.takeColumn(samples.columnIndex("batch number"));
		samples.takeColumn(samples.columnIndex("resequencing"));
	}

	//show table in GUI
	QStringList quality_values = samples.takeColumn(samples.columnIndex("quality"));
	ui_->samples->setData(samples);
	ui_->samples->setQualityIcons("sample", quality_values);
	ui_->samples->setColumnWidth(ui_->samples->columnIndex("comments"), 350);

	//colors
	QColor orange = QColor(255,150,0,125);
	QColor red = QColor(255,0,0,125);
	if (ui_->show_qc_cols->isChecked())
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
				if (is_wgs_run)
				{
					ui_->samples->setBackgroundColorIfLt(header, orange, 35);
					ui_->samples->setBackgroundColorIfLt(header, red, 30);
				}
				else
				{
					ui_->samples->setBackgroundColorIfLt(header, orange, 80);
					ui_->samples->setBackgroundColorIfLt(header, red, 30);
				}
			}
			if (accession=="QC:2000113") //CNV count
			{
				ui_->samples->setBackgroundColorIfLt(header, red, 1);
			}
			if (accession=="QC:2000027") //cov 20x
			{
				if (is_wgs_run)
				{
					ui_->samples->setBackgroundColorIfLt(header, orange, 99);
					ui_->samples->setBackgroundColorIfLt(header, red, 95);
				}
				else
				{
					ui_->samples->setBackgroundColorIfLt(header, orange, 95);
					ui_->samples->setBackgroundColorIfLt(header, red, 90);
				}
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
			if (accession=="QC:2000071") //target region read depth 2-fold duplication
			{
				ui_->samples->setBackgroundColorIfLt(header, orange, 1000);
				ui_->samples->setBackgroundColorIfLt(header, red, 500);
			}
			if (accession=="QC:2000083") //cfDNA-tumor correlation
			{
				ui_->samples->setBackgroundColorIfLt(header, orange, 0.9);
				ui_->samples->setBackgroundColorIfLt(header, red, 0.75);
			}
		}
	}
	ui_->samples->setBackgroundColorIfEqual("is_tumor", orange, "yes");
	ui_->samples->setBackgroundColorIfEqual("is_ffpe", orange, "yes");
	if (ui_->show_lab_cols->isChecked()) ui_->samples->setBackgroundColorIfEqual("resequencing", red, "yes");

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

void SequencingRunWidget::scheduleForResequencing()
{
	NGSD db;

	//prepare query
	SqlQuery query = db.getQuery();
	query.prepare("UPDATE processed_sample SET scheduled_for_resequencing=TRUE WHERE id=:0");

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
	//init
	NGSD db;

	//determine selected processed sample IDs
	QStringList selected_ps_ids;
	QList<int> selected_rows = ui_->samples->selectedRows().toList();
	foreach(int row, selected_rows)
	{
		selected_ps_ids << ui_->samples->getId(row);
	}
	if (selected_rows.isEmpty()) //no selection > select all samples (works if all have the sample processing system)
	{
		selected_ps_ids << db.getValues("SELECT id FROM processed_sample WHERE sequencing_run_id='" + run_id_ + "'");
	}

	//check one processing system is selected
	QStringList system_ids = db.getValues("SELECT DISTINCT processing_system_id FROM processed_sample WHERE id IN ('" + selected_ps_ids.join("', '") + "')");
	if (system_ids.count()!=1)
	{
		QMessageBox::information(this, "Plot error", "Please select one or several samples of the <b>same processing system</b> for plotting!");
		return;
	}

	//create widget
	DBQCWidget* qc_widget = new DBQCWidget();
	qc_widget->setSystemId(system_ids[0]);

	//highlight selected samples
	foreach(QString ps_id, selected_ps_ids)
	{
		qc_widget->addHighlightedProcessedSampleById(ps_id, db.processedSampleName(ps_id), false);
	}

	//determine QC term id
	QString accession = qobject_cast<QAction*>(sender())->data().toString();
	QString qc_term_id = db.getValue("SELECT id FROM qc_terms WHERE qcml_id='" + accession + "'", true).toString();
	qc_widget->setTermId(qc_term_id);

	//show widget
	auto dlg = GUIHelper::createDialog(qc_widget, "QC plot");
	emit addModelessDialog(dlg, false);
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

			int ps_count = db.getValue("SELECT count(id) FROM processed_sample WHERE sequencing_run_id='" + run_id_ + "' AND project_id='" + query.value("id").toString() +"'").toInt();
			body << "  Proben: " + QString::number(ps_count);
			body << "  Analyse: " + query.value("analysis").toString();

			QStringList operator_ids = db.getValues("SELECT operator_id FROM processed_sample WHERE sequencing_run_id='" + run_id_ + "' AND project_id='" + query.value("id").toString() + "' AND operator_id IS NOT NULL");
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

void SequencingRunWidget::exportSampleSheet()
{
	NGSD db;
	try
	{
		QString output_path = Settings::string("sample_sheet_path") + "/" + ui_->name->text().remove(0, 1) + ".csv";
		QStringList warnings;
		QString sample_sheet = db.createSampleSheet(Helper::toInt(run_id_, "Sequencing run id"), warnings);

		if (warnings.size() > 0)
		{
			QMessageBox::StandardButton reply;
			reply = QMessageBox::question(this, "Warning during SampleSheet export",
										  "During the export of the SampleSheet the following warnings occured:\n" + warnings.join("\n") + "\n Do you want to continue?",
										  QMessageBox::Yes|QMessageBox::No);
			// Abort if 'No' was clicked
			if (reply== QMessageBox::No) return;
		}

		QSharedPointer<QFile> output_file = Helper::openFileForWriting(output_path);
		output_file->write(sample_sheet.toLatin1());
		output_file->flush();
		output_file->close();

		QMessageBox::information(this, "SampleSheet exported", "SampleSheet for NovaSeq X Plus exported!");
	}
	catch (Exception& e)
	{
		QMessageBox::warning(this, "SampleSheet export failed", e.message());
	}
}

void SequencingRunWidget::setQCMetricAccessions(const QSet<QString>& sample_types)
{
	// determine QC parameter based on sample types
	qc_metric_accessions_.clear();
	qc_metric_accessions_ << "QC:2000005"; // read count
	qc_metric_accessions_ << "QC:2000023"; // insert size
	qc_metric_accessions_ << "QC:2000021"; // on-target read percentage
	qc_metric_accessions_ << "QC:2000024"; // duplicate read percentage
	qc_metric_accessions_ << "QC:2000025"; // target region read depth
	if (sample_types.contains("DNA") || sample_types.contains("RNA") || sample_types.contains("DNA (amplicon)") || sample_types.contains("DNA (native)"))
	{
		qc_metric_accessions_ << "QC:2000027"; // target region 20x percentage
	}
	if (sample_types.contains("cfDNA"))
	{
		qc_metric_accessions_ << "QC:2000065"; // target region 1000x percentage
		qc_metric_accessions_ << "QC:2000071"; // target region read depth 2-fold duplication
		qc_metric_accessions_ << "QC:2000073"; // target region read depth 4-fold duplication
	}
	if (sample_types.contains("RNA"))
	{
		qc_metric_accessions_ << "QC:2000101"; // housekeeping genes read depth
		qc_metric_accessions_ << "QC:2000102"; // housekeeping genes 10x percentage
	}
	if (sample_types.contains("DNA") || sample_types.contains("DNA (amplicon)") || sample_types.contains("DNA (native)"))
	{
		qc_metric_accessions_ << "QC:2000013"; // variant count
		qc_metric_accessions_ << "QC:2000014"; // known variants percentage
	}
	if (sample_types.contains("DNA") || sample_types.contains("RNA") || sample_types.contains("DNA (amplicon)") || sample_types.contains("DNA (native)"))
	{
		qc_metric_accessions_ << "QC:2000051"; // SNV allele frequency deviation
	}
	if (sample_types.contains("DNA") || sample_types.contains("DNA (amplicon)") || sample_types.contains("DNA (native)"))
	{
		qc_metric_accessions_ << "QC:2000113"; // CNV count
		qc_metric_accessions_ << "QC:2000114"; // CNV coverage profile correlation
	}

	if (sample_types.contains("RNA"))
	{
		qc_metric_accessions_ << "QC:2000109"; // covered gene count
		qc_metric_accessions_ << "QC:2000110"; // aberrant spliced gene count
		qc_metric_accessions_ << "QC:2000111"; // outlier gene count
	}
	if (sample_types.contains("cfDNA"))
	{
		qc_metric_accessions_ << "QC:2000077"; // monitoring variant read depth
		qc_metric_accessions_ << "QC:2000079"; // monitoring variant count
		qc_metric_accessions_ << "QC:2000080"; // monitoring variant 250x percentage
		qc_metric_accessions_ << "QC:2000083"; // cfDNA-tumor correlation
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
	headers << "read" << "lane" << "Q30 [%]" << "error rate [%]" << "occupied [%]" << "clusters [K/mm2]" << "clusters PF [%]" << "yield [GB]";

	table->setColumnCount(headers.count());
	for(int c=0; c<headers.count(); ++c)
	{
		table->setHorizontalHeaderItem(c, GUIHelper::createTableItem(headers[c], Qt::AlignCenter));
	}

	//add QC of reads/lanes
	NGSD db;
	SqlQuery q_reads = db.getQuery();
	q_reads.exec("SELECT * FROM runqc_read WHERE sequencing_run_id=" + run_id_ + " ORDER BY read_num ASC");
	QHash<QString, QVector<double>> type2values;
	while(q_reads.next())
	{
		//all lanes summary line
		int row = table->rowCount();
		table->setRowCount(row + 1);

		QString text = "R" + q_reads.value("read_num").toString() + " (" + q_reads.value("cycles").toString() + "cycles";
		bool is_index = q_reads.value("is_index").toBool();
		if (is_index) text += ", index";
		text += ")";
		QTableWidgetItem* item = GUIHelper::createTableItem(text);
		highlightItem(item);
		table->setItem(row, 0, item);

		item = GUIHelper::createTableItem(QString("all"));
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

			table->setItem(row, 0, GUIHelper::createTableItem(QString()));
			table->setItem(row, 1, GUIHelper::createTableItem(q_lanes.value("lane_num").toString()));
			double q30 = q_lanes.value("q30_perc").toDouble();
			table->setItem(row, 2, GUIHelper::createTableItem(QString::number(q30, 'f', 2)));
			if (!is_index)type2values["q30"] << q30;
			double error_rate = q_lanes.value("error_rate").toDouble();
			table->setItem(row, 3, GUIHelper::createTableItem(QString::number(error_rate, 'f', 2)));
			if (!is_index) type2values["error_rate"] << error_rate;
			table->setItem(row, 4, GUIHelper::createTableItem(QString::number(q_lanes.value("occupied_perc").toDouble(), 'f', 2)));
			double density = q_lanes.value("cluster_density").toDouble();
			table->setItem(row, 5, GUIHelper::createTableItem(QString::number(density/1000.0, 'f', 2)));
			table->setItem(row, 6, GUIHelper::createTableItem(QString::number(q_lanes.value("cluster_density_pf").toDouble()/density*100.0, 'f', 2)));
			double yield_gb = q_lanes.value("yield").toDouble()/1000000000.0;
			table->setItem(row, 7, GUIHelper::createTableItem(QString::number(yield_gb, 'f', 2)));
			type2values["yield"] << yield_gb;
		}
	}

	//add QC summary
	if (!type2values.isEmpty())
	{
		int row = table->rowCount();
		table->setRowCount(row + 1);

		QTableWidgetItem* item = GUIHelper::createTableItem("overall");
		highlightItem(item);
		table->setItem(row, 0, item);

		item = GUIHelper::createTableItem(QString::number(BasicStatistics::mean(type2values["q30"]), 'f', 2));
		highlightItem(item);
		table->setItem(row, 2, item);

		item = GUIHelper::createTableItem(QString::number(BasicStatistics::mean(type2values["error_rate"]), 'f', 2));
		highlightItem(item);
		table->setItem(row, 3, item);

		item = GUIHelper::createTableItem(QString::number(std::accumulate(type2values["yield"].begin(), type2values["yield"].end(), 0.0), 'f', 2));
		highlightItem(item);
		table->setItem(row, 7, item);
	}

	GUIHelper::resizeTableCells(table);
}

void SequencingRunWidget::openSelectedSampleTabs()
{
	int col = ui_->samples->columnIndex("sample");
	QList<int> selected_rows = ui_->samples->selectedRows().toList();
	foreach (int row, selected_rows)
	{
		QString ps = ui_->samples->item(row, col)->text();
		GlobalServiceProvider::openProcessedSampleTab(ps);
	}
}

void SequencingRunWidget::openSampleTab(int row)
{
	int col = ui_->samples->columnIndex("sample");
	QString ps = ui_->samples->item(row, col)->text();
	GlobalServiceProvider::openProcessedSampleTab(ps);
}
