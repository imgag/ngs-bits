#include "ProcessedSampleWidget.h"
#include "ui_ProcessedSampleWidget.h"
#include "DBQCWidget.h"
#include "GUIHelper.h"

#include <QMessageBox>

ProcessedSampleWidget::ProcessedSampleWidget(QWidget* parent, QString ps_id)
	: QWidget(parent)
	, ui_(new Ui::ProcessedSampleWidget)
	, ps_id_(ps_id)
{
	ui_->setupUi(this);
	GUIHelper::styleSplitter(ui_->splitter);
	connect(ui_->ngsd_btn, SIGNAL(clicked(bool)), this, SLOT(openSampleInNGSD()));
	connect(ui_->run, SIGNAL(linkActivated(QString)), this, SIGNAL(openRunTab(QString)));

	QAction* action = new QAction("Plot", this);
	ui_->qc_table->addAction(action);
	connect(action, SIGNAL(triggered(bool)), this, SLOT(showPlot()));

	action = new QAction("Open processed sample tab", this);
	ui_->sample_relations->addAction(action);
	connect(action, SIGNAL(triggered(bool)), this, SLOT(openSampleTab()));

	connect(ui_->qc_all, SIGNAL(stateChanged(int)), this, SLOT(updateQCMetrics()));

	updateGUI();
}

ProcessedSampleWidget::~ProcessedSampleWidget()
{
	delete ui_;
}

void ProcessedSampleWidget::updateGUI()
{
	//#### processed sample details ####
	ProcessedSampleData ps_data = db_.getProcessedSampleData(ps_id_);
	ui_->name->setText(ps_data.name);
	ui_->comments->setText(ps_data.comments);
	ui_->system->setText(ps_data.processing_system);
	ui_->project->setText(ps_data.project_name);
	QString quality = ps_data.quality;
	if (quality=="bad") quality = "<font color=red>bad</font>";
	ui_->quality->setText(quality);
	QString run = ps_data.run_name;
	ui_->run->setText("<a href=\"" + run + "\">"+run+"</a>");

	//#### sample details ####
	QString s_id = db_.getValue("SELECT sample_id FROM processed_sample WHERE id='" + ps_id_ + "'").toString();
	SampleData s_data = db_.getSampleData(s_id);
	ui_->name_external->setText(s_data.name_external);
	ui_->gender->setText(s_data.gender);
	ui_->tumor_ffpe->setText(QString(s_data.is_tumor ? "<font color=red>yes</font>" : "no") + " / " + (s_data.is_ffpe ? "<font color=red>yes</font>" : "no"));
	ui_->disease_group->setText(s_data.disease_group);
	ui_->disease_status->setText(s_data.disease_status);
	ui_->s_comments->setText(s_data.comments);
	ui_->s_name->setText(s_data.name);
	quality = s_data.quality;
	if (quality=="bad") quality = "<font color=red>bad</font>";
	ui_->s_quality->setText(quality);

	//#### disease details ####
	DBTable dd_table = db_.createTable("sample_disease_info", "SELECT sdi.id, sdi.type, sdi.disease_info, u.name, sdi.date, t.name as hpo_name FROM user u, processed_sample ps, sample_disease_info sdi LEFT JOIN hpo_term t ON sdi.disease_info=t.hpo_id WHERE sdi.sample_id=ps.sample_id AND sdi.user_id=u.id AND ps.id='" + ps_id_ + "' ORDER BY sdi.date ASC");
	//append merge HPO id and name
	QStringList hpo_names = dd_table.takeColumn(dd_table.columnIndex("hpo_name"));
	QStringList info_entries = dd_table.extractColumn(dd_table.columnIndex("disease_info"));
	for (int i=0; i<info_entries.count(); ++i)
	{
		if (info_entries[i].startsWith("HP:"))
		{
			info_entries[i] += " (" + hpo_names[i] + ")";
		}
	}
	dd_table.setColumn(dd_table.columnIndex("disease_info"), info_entries);
	ui_->disease_details->setData(dd_table);


	//#### sample relations ####
	DBTable rel_table = db_.createTable("sample_relations", "SELECT id, (SELECT name FROM sample WHERE id=sample1_id), relation, (SELECT name FROM sample WHERE id=sample2_id) FROM sample_relations WHERE sample1_id='" + s_id + "' OR sample2_id='" + s_id + "'");
	rel_table.setHeaders(QStringList() << "sample1" << "relation" << "sample2");
	ui_->sample_relations->setData(rel_table);

	//#### QC ####
	updateQCMetrics();
}

void ProcessedSampleWidget::updateQCMetrics()
{
	QString conditions;
	if (!ui_->qc_all->isChecked())
	{
		conditions = "AND (t.qcml_id='QC:2000007' OR 'QC:2000008' OR t.qcml_id='QC:2000010' OR t.qcml_id='QC:2000013' OR t.qcml_id='QC:2000014' OR t.qcml_id='QC:2000015' OR t.qcml_id='QC:2000016' OR t.qcml_id='QC:2000017' OR t.qcml_id='QC:2000018' OR t.qcml_id='QC:2000020' OR t.qcml_id='QC:2000021' OR t.qcml_id='QC:2000022' OR t.qcml_id='QC:2000023' OR t.qcml_id='QC:2000024' OR t.qcml_id='QC:2000025' OR t.qcml_id='QC:2000027' OR t.qcml_id='QC:2000049' OR t.qcml_id='QC:2000050' OR t.qcml_id='QC:2000051')";
	}

	DBTable qc_table = db_.createTable("processed_sample_qc", "SELECT qc.id, t.qcml_id, t.name, qc.value, t.description FROM processed_sample_qc qc, qc_terms t WHERE qc.qc_terms_id=t.id AND t.obsolete=0 AND qc.processed_sample_id='" + ps_id_ + "' " + conditions + " ORDER BY t.qcml_id ASC");
	//use descriptions as tooltip
	QStringList descriptions = qc_table.takeColumn(qc_table.columnIndex("description"));
	ui_->qc_table->setData(qc_table);
	ui_->qc_table->setColumnTooltips(qc_table.columnIndex("name"), descriptions);
}

void ProcessedSampleWidget::showPlot()
{
	QList<int> selected_rows = ui_->qc_table->selectedRows().toList();
	if (selected_rows.count()>2)
	{
		QMessageBox::information(this, "Plot error", "Please select <b>one or two</b> quality metric for plotting!");
		return;
	}

	//get database IDs
	QString qc_term_id = db_.getValue("SELECT qc_terms_id FROM processed_sample_qc WHERE id='" + ui_->qc_table->getId(selected_rows[0]) + "'").toString();
	QString sys_id = db_.getValue("SELECT ps.processing_system_id FROM processed_sample_qc qc, processed_sample ps WHERE qc.processed_sample_id=ps.id AND qc.id='" + ui_->qc_table->getId(selected_rows[0]) + "'").toString();


	//show widget
	DBQCWidget* qc_widget = new DBQCWidget(this);
	qc_widget->addHighlightedProcessedSampleById(ps_id_);
	qc_widget->setSystemId(sys_id);
	qc_widget->setTermId(qc_term_id);
	if (selected_rows.count()==2)
	{
		QString qc_term_id2 = db_.getValue("SELECT qc_terms_id FROM processed_sample_qc WHERE id='" + ui_->qc_table->getId(selected_rows[1]) + "'").toString();
		qc_widget->setSecondTermId(qc_term_id2);
	}
	auto dlg = GUIHelper::createDialog(qc_widget, "QC plot");
	dlg->exec();
}

void ProcessedSampleWidget::openSampleInNGSD()
{
	try
	{
		QString url = NGSD().url(ui_->name->text());
		QDesktopServices::openUrl(QUrl(url));
	}
	catch (DatabaseException e)
	{
		GUIHelper::showMessage("NGSD error", "Error message: " + e.message());
		return;
	}
}

void ProcessedSampleWidget::openSampleTab()
{
	QString s_name = db_.getValue("SELECT s.name FROM sample s, processed_sample ps WHERE ps.sample_id=s.id AND ps.id='" + ps_id_ + "'").toString();

	QStringList ps_names;
	QList<int> selected_rows = ui_->sample_relations->selectedRows().toList();
	foreach(int row, selected_rows)
	{
		QString s = ui_->sample_relations->item(row, 0)->text();
		if (s==s_name)
		{
			s = ui_->sample_relations->item(row, 2)->text();
		}

		QStringList tmp = db_.getValues("SELECT ps.id FROM processed_sample ps, sample s WHERE ps.sample_id=s.id AND s.name='" + s + "'");
		foreach(QString ps_id, tmp)
		{
			ps_names << db_.processedSampleName(ps_id);
		}
	}

	if (ps_names.count()==0)
	{
		QMessageBox::information(this, "No sample", "No processed sample found for this sample!");
	}
	else if (ps_names.count()==1)
	{
		emit openProcessedSampleTab(ps_names[0]);
	}
	else if (ps_names.count()>0)
	{
		bool ok;
		QString ps = QInputDialog::getItem(this, "Select processed sample", "sample:", ps_names, 0, false, &ok);
		if (ok)
		{
			emit openProcessedSampleTab(ps);
		}
	}
}

