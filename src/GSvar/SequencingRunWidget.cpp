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
#include "NsxSettingsDialog.h"
#include <numeric>
#include <QSignalMapper>
#include <QInputDialog>
#include <QMessageBox>

SequencingRunWidget::SequencingRunWidget(QWidget* parent, const QStringList& run_ids)
	: TabBaseClass(parent)
	, ui_(new Ui::SequencingRunWidget)
	, run_ids_(run_ids)
{
	if (run_ids_.size() < 1) THROW(ArgumentException, "At least one run_id has to be provided!");
	is_batch_view_ = run_ids.size() > 1;

	run_ids_.sort();
	ui_->setupUi(this);
	GUIHelper::styleSplitter(ui_->splitter);
	ui_->splitter->setSizes(QList<int>() << 200 << 800);
	connect(ui_->show_qc_cols, SIGNAL(toggled(bool)), this, SLOT(updateGUI()));
	connect(ui_->show_lab_cols, SIGNAL(toggled(bool)), this, SLOT(updateGUI()));
	connect(ui_->show_disease_cols, SIGNAL(toggled(bool)), this, SLOT(updateGUI()));
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

	if (is_batch_view_) initBatchView();

	updateGUI();
	ui_->gb_sequencing_run->setStyleSheet("QGroupBox {background-color: white;}");
}

SequencingRunWidget::~SequencingRunWidget()
{
	delete ui_;
}

void SequencingRunWidget::initBatchView()
{
	if (!is_batch_view_) return;
	try
	{
		//clear default view
		QLayoutItem* item;
		while (ui_->gb_sequencing_run->layout()->count() > 0)
		{
			item = ui_->gb_sequencing_run->layout()->takeAt(0);
			delete item->widget();
			delete item;
		}
		delete ui_->name;
		delete ui_->quality;
		delete ui_->gb_sequencing_run->layout();

		//replace sequencing run list with tablular view
		QGridLayout* seq_run_table = new QGridLayout();

		//create vertical header
		int r = 0;
		seq_run_table->addWidget(new QLabel("name:"), r++, 0, Qt::AlignLeft);
		seq_run_table->addWidget(new QLabel("flowcell ID:"), r++, 0, Qt::AlignLeft);
		seq_run_table->addWidget(new QLabel("flowcell type:"), r++, 0, Qt::AlignLeft);
		seq_run_table->addWidget(new QLabel("device:"), r++, 0, Qt::AlignLeft);
		seq_run_table->addWidget(new QLabel("side:"), r++, 0, Qt::AlignLeft);
		seq_run_table->addWidget(new QLabel("recipe:"), r++, 0, Qt::AlignLeft);
		seq_run_table->addWidget(new QLabel("start date:"), r++, 0, Qt::AlignLeft);
		seq_run_table->addWidget(new QLabel("end date:"), r++, 0, Qt::AlignLeft);
		seq_run_table->addWidget(new QLabel("pool molarity:"), r++, 0, Qt::AlignLeft);
		seq_run_table->addWidget(new QLabel("status:"), r++, 0, Qt::AlignLeft);
		seq_run_table->addWidget(new QLabel("comments:"), r++, 0, Qt::AlignLeft|Qt::AlignTop);
		seq_run_table->addWidget(new QLabel("backup done:"), r++, 0, Qt::AlignLeft);
		seq_run_table->addItem(new QSpacerItem(10, 20, QSizePolicy::Minimum, QSizePolicy::Expanding), r++, 0);


		for (int c = 1; c <= run_ids_.size(); ++c)
		{
			r = 0; //reset row counter
			QHBoxLayout* hbox = new QHBoxLayout();
			QLabel* l_quality = new QLabel();
			ProcessedSampleWidget::styleQualityLabel(l_quality, "n/a");
			l_quality->setMaximumWidth(19);
			hbox->addWidget(l_quality, 0, Qt::AlignLeft| Qt::AlignCenter);
			QLabel* l_name = new QLabel("name");
			l_name->setAlignment(Qt::AlignLeft| Qt::AlignCenter);
			hbox->addWidget(l_name, 0, Qt::AlignLeft| Qt::AlignCenter);

			QToolButton* btn_edit = new QToolButton();
			btn_edit->setText("");
			btn_edit->setIcon(QIcon(":/Icons/Edit.png"));
			btn_edit->setIconSize(QSize(14,14));
			btn_edit->setFixedWidth(19);
			btn_edit->setFixedHeight(19);

			//link button through signal mapper
			QSignalMapper* signal_mapper = new QSignalMapper(this);
			int run_id = run_ids_.at(c-1).toInt();
			signal_mapper->setMapping(btn_edit, run_id);
			connect(btn_edit, SIGNAL(clicked(bool)), signal_mapper, SLOT(map()));
			connect(signal_mapper, SIGNAL(mapped(int)), this, SLOT(edit(int)));

			hbox->addWidget(btn_edit, 0, Qt::AlignLeft| Qt::AlignCenter);

			seq_run_table->addItem(hbox, r++, c, Qt::AlignLeft);
			seq_run_table->addWidget(new QLabel("fcid"), r++, c, Qt::AlignLeft);
			seq_run_table->addWidget(new QLabel("flowcell_type"), r++, c, Qt::AlignLeft);
			seq_run_table->addWidget(new QLabel("d_name"), r++, c, Qt::AlignLeft);
			seq_run_table->addWidget(new QLabel("side"), r++, c, Qt::AlignLeft);
			seq_run_table->addWidget(new QLabel("recipe"), r++, c, Qt::AlignLeft);
			seq_run_table->addWidget(new QLabel("start_date"), r++, c, Qt::AlignLeft);
			seq_run_table->addWidget(new QLabel("end_date"), r++, c, Qt::AlignLeft);
			seq_run_table->addWidget(new QLabel("molarity"), r++, c, Qt::AlignLeft);
			seq_run_table->addWidget(new QLabel("status"), r++, c, Qt::AlignLeft);
			QLabel* comments = new QLabel("comments");
			comments->setAlignment(Qt::AlignLeft|Qt::AlignTop);
			comments->setMaximumWidth(200);
			seq_run_table->addWidget(comments, r++, c, Qt::AlignLeft|Qt::AlignTop);
			seq_run_table->addWidget(new QLabel("backup_done"), r++, c, Qt::AlignLeft);
			seq_run_table->setColumnMinimumWidth(c, 150);

		}
		seq_run_table->addItem(new QSpacerItem(20, 10, QSizePolicy::Expanding, QSizePolicy::Minimum), 0, run_ids_.size()+1);


		ui_->gb_sequencing_run->setLayout(seq_run_table);
		ui_->gb_sequencing_run->setStyleSheet("QGroupBox {background-color: white;}");
		ui_->gb_sequencing_run->setMinimumHeight(280);



		//disable edit/MID buttons in batch view
		ui_->edit_btn->setEnabled(false);
		ui_->mid_check_btn->setEnabled(false);

	}
	catch (Exception& e)
	{
		QMessageBox::warning(this, "Batch view init failed!", "Could not initialize widget:\n" + e.message());
	}
}

void SequencingRunWidget::updateGUI()
{
	is_busy_ = true;

	try
	{
		//#### run details ####
		NGSD db;
		SqlQuery query = db.getQuery();
		if (is_batch_view_)
		{

			//update data
			int r;
			for (int c = 1; c <= run_ids_.size(); ++c)
			{
				r = 0; //reset row counter
				query.exec("SELECT r.*, d.name d_name, d.type d_type FROM sequencing_run r, device d WHERE r.device_id=d.id AND r.id='" + run_ids_.at(c-1) + "'");
				query.next();

				QGridLayout* seq_run_table = (QGridLayout*) ui_->gb_sequencing_run->layout();

				QHBoxLayout* hbox = (QHBoxLayout*) seq_run_table->itemAtPosition(r++, c);
				QLabel* l_quality = (QLabel*) hbox->itemAt(0)->widget();
				ProcessedSampleWidget::styleQualityLabel(l_quality, query.value("quality").toString());
				QLabel* l_name = (QLabel*) hbox->itemAt(1)->widget();
				l_name->setText(query.value("name").toString());

				((QLabel*) seq_run_table->itemAtPosition(r++, c)->widget())->setText(query.value("fcid").toString());
				((QLabel*) seq_run_table->itemAtPosition(r++, c)->widget())->setText(query.value("flowcell_type").toString());
				((QLabel*) seq_run_table->itemAtPosition(r++, c)->widget())->setText(query.value("d_name").toString() + " (" + query.value("d_type").toString() + ")");
				((QLabel*) seq_run_table->itemAtPosition(r++, c)->widget())->setText(query.value("side").toString());
				((QLabel*) seq_run_table->itemAtPosition(r++, c)->widget())->setText(query.value("recipe").toString());
				((QLabel*) seq_run_table->itemAtPosition(r++, c)->widget())->setText(query.value("start_date").toString());
				((QLabel*) seq_run_table->itemAtPosition(r++, c)->widget())->setText(query.value("end_date").toString());
				QVariant molarity = query.value("pool_molarity");
				((QLabel*) seq_run_table->itemAtPosition(r++, c)->widget())->setText((molarity.isNull())?(molarity.toString() + " (" + query.value("pool_quantification_method").toString() + ")"):(""));
				if (query.value("status").toString()=="analysis_finished")((QLabel*) seq_run_table->itemAtPosition(r++, c)->widget())->setText(query.value("status").toString());
				else ((QLabel*) seq_run_table->itemAtPosition(r++, c)->widget())->setText("<font color=orange>" + query.value("status").toString() + "</font>");
				GSvarHelper::limitLines((QLabel*) seq_run_table->itemAtPosition(r++, c)->widget(), query.value("comment").toString());
				((QLabel*) seq_run_table->itemAtPosition(r++, c)->widget())->setText((query.value("backup_done").toString()=="1" ? "yes" : "<font color=red>no</font>"));

			}

			//#### deactivate SampleSheet ####
			ui_->novaseqx_samplesheet_btn->setEnabled(false);

		}
		else
		{
			query.exec("SELECT r.*, d.name d_name, d.type d_type FROM sequencing_run r, device d WHERE r.device_id=d.id AND r.id='" + run_ids_.at(0) + "'");
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
		}

		//#### run quality ####
		updateReadQualityTable();

		//#### sample table ####
		updateRunSampleTable();

	}
	catch (Exception& e)
	{
		QMessageBox::warning(this, "Update failed", "Could not update data:\n" + e.message());
	}

	is_busy_ = false;
}

void SequencingRunWidget::updateRunSampleTable()
{
	//get data from NGSD
	QStringList headers;
	if (is_batch_view_) headers << "run";
	headers << "lane" << "quality" << "sample" << "name external" << "is_tumor" << "is_ffpe" << "sample type" << "project" << "disease group" << "disease status" << "MID i7" << "MID i5" << "species" << "processing system" << "input [ng]" << "molarity [nM]" << "operator" << "processing modus" << "batch number" << "comments";
	if (ui_->show_sample_comment->isChecked())
	{
		headers << "sample comments";
	}
	headers << "urgent";
	headers << "resequencing";
	NGSD db;
	DBTable samples;
	if (is_batch_view_)
	{
		samples = db.createTable("processed_sample",  QString("SELECT ps.id, (SELECT name FROM sequencing_run WHERE id=ps.sequencing_run_id), ps.lane, ps.quality, ")
														+ "CONCAT(s.name,'_',LPAD(ps.process_id,2,'0')), s.name_external, s.tumor, s.ffpe, s.gender, s.sample_type, "
														+ "(SELECT CONCAT(name, ' (', type, ')') FROM project WHERE id=ps.project_id), s.disease_group, s.disease_status, "
														+ "(SELECT CONCAT(name, ' (', sequence, ')') FROM mid WHERE id=ps.mid1_i7), (SELECT CONCAT(name, ' (', sequence, ')') FROM mid WHERE id=ps.mid2_i5), "
														+ "sp.name, sys.name_manufacturer, sys.type as sys_type, ps.processing_input, ps.molarity, (SELECT name FROM user WHERE id=ps.operator_id), "
														+ "ps.processing_modus, ps.batch_number, ps.comment" + QString(ui_->show_sample_comment->isChecked() ? ", s.comment as sample_comment" : "")
														+ ",ps.urgent ,ps.scheduled_for_resequencing FROM processed_sample ps, sample s, processing_system sys, species sp "
														+ "WHERE sp.id=s.species_id AND ps.processing_system_id=sys.id AND ps.sample_id=s.id AND ps.sequencing_run_id IN ('" + run_ids_.join("', '") + "') "
														+ "ORDER BY ps.lane ASC, "+ (ui_->sort_by_ps_id->isChecked() ? "ps.id" : "ps.sequencing_run_id ASC, ps.processing_system_id ASC, s.name ASC, ps.process_id"));
	}
	else
	{
		samples = db.createTable("processed_sample", QString("SELECT ps.id, ps.lane, ps.quality, CONCAT(s.name,'_',LPAD(ps.process_id,2,'0')), s.name_external, s.tumor, s.ffpe, s.gender, s.sample_type, (SELECT CONCAT(name, ' (', type, ')') ")
													+ "FROM project WHERE id=ps.project_id), s.disease_group, s.disease_status, (SELECT CONCAT(name, ' (', sequence, ')') FROM mid WHERE id=ps.mid1_i7), (SELECT CONCAT(name, ' (', sequence, ')') "
													+ "FROM mid WHERE id=ps.mid2_i5), sp.name, sys.name_manufacturer, sys.type as sys_type, ps.processing_input, ps.molarity, (SELECT name FROM user WHERE id=ps.operator_id), ps.processing_modus, ps.batch_number, ps.comment" + QString(ui_->show_sample_comment->isChecked() ? ", s.comment as sample_comment" : "") + ", ps.urgent, ps.scheduled_for_resequencing "
													+ "FROM processed_sample ps, sample s, processing_system sys, species sp WHERE sp.id=s.species_id AND ps.processing_system_id=sys.id AND ps.sample_id=s.id AND ps.sequencing_run_id IN ('" + run_ids_.join("', '") + "') "
													+ "ORDER BY ps.lane ASC, "+ (ui_->sort_by_ps_id->isChecked() ? "ps.id" : "ps.processing_system_id ASC, s.name ASC, ps.process_id"));
	}	//format columns
	samples.formatBooleanColumn(samples.columnIndex("tumor"));
	samples.formatBooleanColumn(samples.columnIndex("ffpe"));
	samples.formatBooleanColumn(samples.columnIndex("urgent"), true);
	samples.formatBooleanColumn(samples.columnIndex("scheduled_for_resequencing"), true);

	// determine QC parameter based on sample types
    QSet<QString> sample_types = LIST_TO_SET(samples.extractColumn(samples.columnIndex("sample_type")));
    QSet<QString> system_types = LIST_TO_SET(samples.extractColumn(samples.columnIndex("sys_type")));
	setQCMetricAccessions(sample_types, system_types);

	// update QC plot button
	ui_->plot_btn->setMenu(new QMenu());
	foreach(QString accession, qc_metric_accessions_)
	{
		QString name = db.getValue("SELECT name FROM qc_terms WHERE qcml_id=:0", true, accession).toString();
		name.replace("percentage", "%");
		ui_->plot_btn->menu()->addAction(name, this, SLOT(showPlot()))->setData(accession);
	}

	//remove columns not shown but needed for QC
	QStringList genders = samples.takeColumn(samples.columnIndex("gender"));
	QStringList sys_types = samples.takeColumn(samples.columnIndex("sys_type"));

	//add QC data
	QHash<QString, QString> metric2header;
	if (ui_->show_qc_cols->isChecked())
	{
		//create column data
		QList<QStringList> cols;
		while(cols.count()< qc_metric_accessions_.count()) cols << QStringList();
		for (int r=0; r<samples.rowCount(); ++r)
		{
			QCCollection qc_data = db.getQCData(samples.row(r).id());
			for(int i=0; i<qc_metric_accessions_.count(); ++i)
			{
				try
				{
					QString value = qc_data.value(qc_metric_accessions_[i], true).toString();
					cols[i] << value;
				}
				catch(...)
				{
					cols[i] << "";
				}
			}
		}
		//add columns
		for(int i=0; i<qc_metric_accessions_.count(); ++i)
		{
			QString header = db.getValue("SELECT name FROM qc_terms WHERE qcml_id=:0", true, qc_metric_accessions_[i]).toString();
			header.replace("percentage", "%");
			samples.addColumn(cols[i], header);
			headers << header;
			metric2header.insert(qc_metric_accessions_[i], header);

			//add 'read %' column
			if (qc_metric_accessions_[i]=="QC:2000005")
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

	//remove disease columns
	if (!ui_->show_disease_cols->isChecked())
	{
		samples.takeColumn(samples.columnIndex("disease group"));
		samples.takeColumn(samples.columnIndex("disease status"));
	}

	//show table in GUI
	QStringList quality_values = samples.takeColumn(samples.columnIndex("quality"));
	ui_->samples->setData(samples);
	ui_->samples->setQualityIcons("sample", quality_values);

	//colors
	QColor orange = QColor(255,150,0,125);
	QColor red = QColor(255,0,0,125);
	if (ui_->show_qc_cols->isChecked())
	{
		foreach(const QString& accession, qc_metric_accessions_)
		{
			int c = ui_->samples->columnIndex(metric2header[accession]);

			for (int r=0; r<ui_->samples->rowCount(); ++r)
			{
				GSvarHelper::colorQcItem(ui_->samples->item(r,c), accession, sys_types[r], genders[r]);
			}
		}
	}
	ui_->samples->setBackgroundColorIfEqual("is_tumor", orange, "yes");
	ui_->samples->setBackgroundColorIfEqual("is_ffpe", orange, "yes");
	if (ui_->show_lab_cols->isChecked()) ui_->samples->setBackgroundColorIfEqual("resequencing", red, "yes");

	//#### sample summary ####
	QStringList imported_qc, imported_vars;
	imported_qc = db.getValues("SELECT ps.id FROM processed_sample ps WHERE ps.sequencing_run_id IN ('" + run_ids_.join("', '") + "') AND EXISTS(SELECT id FROM processed_sample_qc WHERE processed_sample_id=ps.id)");
	imported_vars = db.getValues("SELECT ps.id FROM processed_sample ps WHERE ps.sequencing_run_id IN ('" + run_ids_.join("', '") + "') AND EXISTS(SELECT variant_id FROM detected_variant WHERE processed_sample_id=ps.id)");
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
    QList<int> selected_rows = ui_->samples->selectedRows().values();
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
    QList<int> selected_rows = ui_->samples->selectedRows().values();
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
    QList<int> selected_rows = ui_->samples->selectedRows().values();
	foreach(int row, selected_rows)
	{
		selected_ps_ids << ui_->samples->getId(row);
	}
	if (selected_rows.isEmpty()) //no selection > select all samples (works if all have the sample processing system)
	{
		if (is_batch_view_)
		{
			selected_ps_ids << db.getValues("SELECT id FROM processed_sample WHERE sequencing_run_id IN ('" + run_ids_.join("', '") + "')");
		}
		else
		{
			selected_ps_ids << db.getValues("SELECT id FROM processed_sample WHERE sequencing_run_id='" + run_ids_.at(0) + "'");
		}
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
	//not working in batch view
	if (is_batch_view_) return;

	DBEditor* widget = new DBEditor(this, "sequencing_run", run_ids_.at(0).toInt());
	auto dlg = GUIHelper::createDialog(widget, "Edit sequencing run " + ui_->name->text() ,"", true);
	if (dlg->exec()==QDialog::Accepted)
	{
		widget->store();
		updateGUI();
	}
}

void SequencingRunWidget::edit(int run_id)
{
	//only working in batch view
	if (!is_batch_view_) return;

	DBEditor* widget = new DBEditor(this, "sequencing_run", run_id);
	QString run_name = NGSD().getValue("SELECT name FROM sequencing_run WHERE id=:0;", false, QString::number(run_id)).toString();

	auto dlg = GUIHelper::createDialog(widget, "Edit sequencing run " + run_name ,"", true);
	if (dlg->exec()==QDialog::Accepted)
	{
		widget->store();
		updateGUI();
	}
}

void SequencingRunWidget::sendStatusEmail()
{
	NGSD db;

	//create email
	QStringList to, body;
	QString subject;
	QString status;
	QString run_name;

	if (is_batch_view_)
	{
		// get run status of all runs
        QSet<QString> run_statuses = LIST_TO_SET(db.getValues("SELECT status FROM sequencing_run WHERE id IN (" + run_ids_.join(", ") + ");"));
		run_statuses.remove("analysis_finished");
		run_statuses.remove("run_finished");

		if(run_statuses.size() > 0 )
		{
			QMessageBox::information(this, "Run status email", "Run status emails can only be generated if all runs have status 'analysis_finished' or 'run_finished'!");
			return;
		}

		QStringList run_names = db.getValues("SELECT name FROM sequencing_run WHERE id IN (" + run_ids_.join(", ") + ");");
		std::sort(run_names.begin(), run_names.end());
		run_name = run_names.first() + " - " + run_names.last();

	}
	else
	{
		status = ui_->status->text();
		if (status!="analysis_finished" && status!="run_finished")
		{
			QMessageBox::information(this, "Run status email", "Run status emails can only be generated for runs with status 'analysis_finished' or 'run_finished'!");
			return;
		}

		run_name = ui_->name->text();
	}

	if (is_batch_view_ || status=="analysis_finished")
	{
		//update table info to be sure it is the current info.
		updateRunSampleTable();

		to << Settings::string("email_run_analyzed").split(";");

		subject = (is_batch_view_)?"[NGSD] Läufe " + run_name + " analysiert": "[NGSD] Lauf " + run_name + " analysiert";

		body << "Hallo zusammen,";
		body << "";
		body << ((is_batch_view_)?"die Läufe " + run_name + " sind fertig analysiert.": "der Lauf " + run_name + " ist fertig analysiert.");

		body << "";

		int idx_sample = ui_->samples->columnIndex("sample");
		int idx_comment = ui_->samples->columnIndex("comments");
		int idx_project = ui_->samples->columnIndex("project");
		int idx_is_tumor = ui_->samples->columnIndex("is_tumor");
		int idx_is_ffpe = ui_->samples->columnIndex("is_ffpe");
		int idx_urgent = ui_->samples->columnIndex("urgent");
		NGSD db;

		//bad/medium samples tables
		QStringList table_bad;
		table_bad << "Probe\tProjekt\tErkrankungsgruppe\tKommentar";
		QStringList table_medium = table_bad;
		for (int i=0; i < ui_->samples->rowCount(); i++)
		{
			if (ui_->samples->item(i, idx_is_tumor)->text() == "yes") continue;

			QString ps_id = db.processedSampleId(ui_->samples->item(i, idx_sample)->text());
			QByteArray quality = db.getValue("SELECT quality FROM processed_sample WHERE id = '" + ps_id + "'").toByteArray();
			if (quality=="bad" || quality=="medium")
			{
				QString ffpe_text = ui_->samples->item(i, idx_is_ffpe)->text() == "yes" ? " [ffpe]" : "";
				QString urgent = ui_->samples->item(i, idx_urgent)->text() == "yes" ? "[eilig] " : "";
				QString ps = ui_->samples->item(i, idx_sample)->text();
				QString disease_group = db.getSampleData(db.sampleId(ps)).disease_group;
				QString line = ps + ffpe_text + "\t" + ui_->samples->item(i, idx_project)->text() + "\t" + disease_group + "\t" + urgent + ui_->samples->item(i, idx_comment)->text().replace("\n", " ");
				if (quality=="bad")
				{
					table_bad << line;
				}
				else
				{
					table_medium << line;
				}
			}
		}

		body << "";
		body << "Die folgenden Keimbahn-Proben des Laufs haben schlechte Qualität:";
		if (table_bad.count() > 1)
		{
			body.append(table_bad);
		}
		else body << "  Keine Proben mit schlechter Qualität.";

		body << "";
		body << "Die folgenden Keimbahn-Proben des Laufs haben mittlere Qualität:";
		if (table_medium.count() > 1)
		{
			body.append(table_medium);
		}
		else body << "Keine Proben mit mittlerer Qualität.";

		//sample summary
		SqlQuery query = db.getQuery();
		query.exec("SELECT DISTINCT p.id, p.name, p.email_notification, p.internal_coordinator_id, p.analysis FROM project p, processed_sample ps WHERE ps.project_id=p.id AND ps.sequencing_run_id IN ('" + run_ids_.join("', '") + "') ORDER BY p.name ASC");
		while(query.next())
		{
			int coordinator_id = query.value("internal_coordinator_id").toInt();
			to << query.value("email_notification").toString().split(";");
			to << db.userEmail(coordinator_id);

			body << "";
			body << "Projekt: " + query.value("name").toString();
			body << "  Koordinator: " + db.userName(coordinator_id);

			int ps_count = db.getValue("SELECT count(id) FROM processed_sample WHERE sequencing_run_id IN ('" + run_ids_.join("', '") + "') AND project_id='" + query.value("id").toString() +"'").toInt();
			body << "  Proben: " + QString::number(ps_count);
			body << "  Analyse: " + query.value("analysis").toString();

			QStringList operator_ids = db.getValues("SELECT operator_id FROM processed_sample WHERE sequencing_run_id IN ('" + run_ids_.join("', '") + "') AND project_id='" + query.value("id").toString() + "' AND operator_id IS NOT NULL");
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
	if (is_batch_view_)
	{
		GUIHelper::showMessage("check MIDs", "MID check is not supported in batch view!");
		return;
	}
	try
	{
		NGSD db;

		//create dialog
		MidCheckWidget* widget = new MidCheckWidget();
		widget->addRun(db.getValue("SELECT name FROM sequencing_run WHERE id=" + run_ids_.at(0)).toString());

		//determine usable length
		QString recipe = db.getValue("SELECT recipe FROM sequencing_run WHERE id=" + run_ids_.at(0)).toString();
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
	if (is_batch_view_)
	{
		GUIHelper::showMessage("SampleSheet export", "SampleSheet export is not supported in batch view!");
		return;
	}
	NGSD db;
	try
	{
		//name
		QString name = ui_->name->text();
		if (name.startsWith("#")) name.remove(0,1);

		//get settings from user
		NsxSettingsDialog dlg(this);
		if (dlg.exec()==QDialog::Rejected) return;
		NsxAnalysisSettings settings = dlg.getSettings();

		//create sample sheet
		QString output_path = Settings::string("sample_sheet_path") + "/" + name + ".csv";
		QStringList warnings;
		QString sample_sheet = db.createSampleSheet(Helper::toInt(run_ids_.at(0), "Sequencing run id"), warnings, settings);

		//show warnings
		if (warnings.size() > 0)
		{
			QMessageBox::StandardButton reply;
			reply = QMessageBox::question(this, "Warning during SampleSheet export",
										  "During the export of the SampleSheet the following warnings occured:\n" + warnings.join("\n") + "\n Do you want to continue?",
										  QMessageBox::Yes|QMessageBox::No);
			// Abort if 'No' was clicked
			if (reply== QMessageBox::No) return;
		}

		//store file
		QSharedPointer<QFile> output_file = Helper::openFileForWriting(output_path);
		output_file->write(sample_sheet.toLatin1());

		QMessageBox::information(this, "SampleSheet exported", "SampleSheet for NovaSeq X Plus written to:\n" +output_path);
	}
	catch (Exception& e)
	{
		QMessageBox::warning(this, "SampleSheet export failed", e.message());
	}
}

void SequencingRunWidget::setQCMetricAccessions(const QSet<QString>& sample_types, const QSet<QString>& system_types)
{
	// determine QC parameter based on sample types
	qc_metric_accessions_.clear();
	qc_metric_accessions_ << "QC:2000005"; // read count
	qc_metric_accessions_ << "QC:2000023"; // insert size
	qc_metric_accessions_ << "QC:2000021"; // on-target read percentage
	qc_metric_accessions_ << "QC:2000024"; // duplicate read percentage
	qc_metric_accessions_ << "QC:2000025"; // target region read depth
	qc_metric_accessions_ << "QC:2000150"; // target region read depth no overlap
	if (sample_types.contains("DNA") || sample_types.contains("RNA") || sample_types.contains("DNA (amplicon)") || sample_types.contains("DNA (native)"))
	{
		qc_metric_accessions_ << "QC:2000027"; // target region 20x percentage
	}
	if (system_types.contains("lrGS"))
	{
		qc_metric_accessions_ << "QC:2000131"; // N50 value
		qc_metric_accessions_ << "QC:2000149"; // Basecall info
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
	table->verticalHeader()->setVisible(false);

	//use different layout depending on sequencer
	NGSD db;
	QString device_type = db.getValue("SELECT d.type FROM sequencing_run r, device d WHERE r.device_id=d.id AND r.id=:0", false, run_ids_.at(0)).toString();
	if (device_type == "PromethION")
	{
		//set headers

		//set horizontal header
		table->setColumnCount(run_ids_.size());
		table->horizontalHeader()->setVisible(true);
		int c = 0;
		foreach (const QString& run_id, run_ids_)
		{
			table->setHorizontalHeaderItem(c++, GUIHelper::createTableItem(db.getValue("SELECT name FROM sequencing_run WHERE id=:0", false, run_id).toString(), Qt::AlignCenter));
		}

		//set vertical header
		QStringList v_headers;
		v_headers << "read number" << "yield [GB]" << "passing filters [%]" << "basecall skipped [%]" << "Q20 [%]" << "Q30 [%]" << "N50";
		table->setRowCount(v_headers.size());
		table->verticalHeader()->setVisible(true);
		for(int r=0; r<v_headers.count(); ++r)
		{
			table->setVerticalHeaderItem(r, GUIHelper::createTableItem(v_headers[r], Qt::AlignLeft));
		}

		//get ONT QC
		c = 0;
		SqlQuery qc_query = db.getQuery();
		foreach (const QString& run_id, run_ids_)
		{
			qc_query.exec("SELECT * FROM runqc_ont WHERE sequencing_run_id=" + run_id);

			if (qc_query.size() > 1)
			{
				THROW(ProgrammingException, "Multiple QC entries found. This should not happen!");
			}
			else if (qc_query.size() == 1)
			{
				qc_query.next();
				int r = 0;
				table->setItem(r++, c, GUIHelper::createTableItem(qc_query.value("read_num").toInt()));
				table->setItem(r++, c, GUIHelper::createTableItem(qc_query.value("yield").toDouble() / 1e9, 2));
				table->setItem(r++, c, GUIHelper::createTableItem(qc_query.value("passing_filter_perc").toDouble(), 2));
				table->setItem(r++, c, GUIHelper::createTableItem(qc_query.value("fraction_skipped").toDouble() * 100.0, 2));
				table->setItem(r++, c, GUIHelper::createTableItem(qc_query.value("q20_perc").toDouble(), 2));
				table->setItem(r++, c, GUIHelper::createTableItem(qc_query.value("q30_perc").toDouble(), 2));
				table->setItem(r++, c, GUIHelper::createTableItem(qc_query.value("n50").toInt()));
				c++;
			}
		}



	}
	else
	{
		if (is_batch_view_) THROW(ArgumentException, "Batch view only supported for PromethION runs!");
		//set header
		QStringList headers;
		headers << "read" << "lane" << "Q30 [%]" << "error rate [%]" << "occupied [%]" << "clusters [K/mm2]" << "clusters PF [%]" << "yield [GB]";

		table->setColumnCount(headers.count());
		for(int c=0; c<headers.count(); ++c)
		{
			table->setHorizontalHeaderItem(c, GUIHelper::createTableItem(headers[c], Qt::AlignCenter));
		}

		//add QC of reads/lanes
		SqlQuery q_reads = db.getQuery();
		q_reads.exec("SELECT * FROM runqc_read WHERE sequencing_run_id=" + run_ids_.at(0) + " ORDER BY read_num ASC");
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
	}

	GUIHelper::resizeTableCellWidths(table);
	GUIHelper::resizeTableCellHeightsToFirst(table);
}

void SequencingRunWidget::openSelectedSampleTabs()
{
	int col = ui_->samples->columnIndex("sample");
    QList<int> selected_rows = ui_->samples->selectedRows().values();
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
