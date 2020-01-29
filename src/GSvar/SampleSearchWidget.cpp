#include "SampleSearchWidget.h"
#include "NGSD.h"
#include "ProcessedSampleDataDeletionDialog.h"
#include "SingleSampleAnalysisDialog.h"
#include <QMessageBox>

SampleSearchWidget::SampleSearchWidget(QWidget* parent)
	: QWidget(parent)
	, ui_()
	, db_()
{
	ui_.setupUi(this);

	//context menu
	QAction* action = new QAction(QIcon(":/Icons/Icon.png"), "Open variant list", this);
	ui_.sample_table->addAction(action);
	connect(action, SIGNAL(triggered(bool)), this, SLOT(openProcessedSample()));
	action = new QAction(QIcon(":/Icons/NGSD_sample.png"), "Open processed sample tab", this);
	ui_.sample_table->addAction(action);
	connect(action, SIGNAL(triggered(bool)), this, SLOT(openProcessedSampleTab()));
	action = new QAction(QIcon(":/Icons/Remove.png"), "Delete associated data", this);
	ui_.sample_table->addAction(action);
	connect(action, SIGNAL(triggered(bool)), this, SLOT(deleteSampleData()));
	action = new QAction(QIcon(":/Icons/reanalysis.png"), "Queue analysis", this);
	ui_.sample_table->addAction(action);
	connect(action, SIGNAL(triggered(bool)), this, SLOT(queueAnalysis()));

	//init search criteria
	//sample
	ui_.s_name->fill(db_.createTable("sample", "SELECT id, name FROM sample"), true);
	ui_.s_species->fill(db_.createTable("species", "SELECT id, name FROM species"), true);
	ui_.s_disease_group->addItem("");
	ui_.s_disease_group->addItems(db_.getEnum("sample", "disease_group"));
	ui_.s_disease_status->addItem("");
	ui_.s_disease_status->addItems(db_.getEnum("sample", "disease_status"));

	//project
	ui_.p_name->fill(db_.createTable("project", "SELECT id, name FROM project"), true);
	ui_.p_type->addItem("");
	ui_.p_type->addItems(db_.getEnum("project", "type"));
	//system
	ui_.sys_name->fill(db_.createTable("processing_system", "SELECT id, name_manufacturer FROM processing_system"), true);
	ui_.sys_type->addItem("");
	ui_.sys_type->addItems(db_.getEnum("processing_system", "type"));
	//run
	ui_.r_name->fill(db_.createTable("sequencing_run", "SELECT id, name FROM sequencing_run"), true);

	//signals/slots
	connect(ui_.search_btn, SIGNAL(clicked(bool)), this, SLOT(search()));
}

void SampleSearchWidget::search()
{
	QApplication::setOverrideCursor(Qt::BusyCursor);

	//init GUI
	ui_.sample_table->clearContents();
	ui_.sample_table->setEnabled(true);
	ui_.search_status->clear();

	//create search parameters
	ProcessedSampleSearchParameters params;
	params.s_name = ui_.s_name->text();
	params.s_species = ui_.s_species->text();
	params.s_disease_group = ui_.s_disease_group->currentText();
	params.s_disease_status = ui_.s_disease_status->currentText();
	params.include_bad_quality_samples = ui_.s_bad_quality->isChecked();
	params.include_tumor_samples = ui_.s_tumor->isChecked();
	params.include_ffpe_samples = ui_.s_ffpe->isChecked();
	params.include_merged_samples = ui_.s_merged->isChecked();

	params.p_name = ui_.p_name->text();
	params.p_type = ui_.p_type->currentText();

	params.sys_name = ui_.sys_name->text();
	params.sys_type = ui_.sys_type->currentText();

	params.r_name = ui_.r_name->text();
	params.include_bad_quality_runs = ui_.r_bad_quality->isChecked();
	params.run_finished = ui_.r_analysis_finished->isChecked();

	params.add_outcome = ui_.add_outcome->isChecked();
	params.add_disease_details = ui_.add_disease_details->isChecked();
	params.add_qc = ui_.add_qc->isChecked();
	params.add_report_config = ui_.add_report_config->isChecked();

	//execute query
	try
	{
		DBTable ps_table = db_.processedSampleSearch(params);
		ui_.sample_table->setData(ps_table);

		//color
		QColor orange = QColor(255,150,0,125);
		QColor red = QColor(255,0,0,125);
		ui_.sample_table->setBackgroundColorIfEqual("quality", orange, "medium");
		ui_.sample_table->setBackgroundColorIfEqual("quality", red, "bad");
		ui_.sample_table->setBackgroundColorIfEqual("run_quality", orange, "medium");
		ui_.sample_table->setBackgroundColorIfEqual("run_quality", red, "bad");

		//text
		ui_.search_status->setText("Found " + QString::number(ps_table.rowCount()) + " matching samples.");
	}
	catch(DatabaseException& e)
	{
		QMessageBox::warning(this, "NGSD error", "Database error:\n" + e.message());
	}

	QApplication::restoreOverrideCursor();
}

void SampleSearchWidget::openProcessedSampleTab()
{

	QSet<int> rows = ui_.sample_table->selectedRows();
	foreach(int row, rows)
	{
		QString ps_id = ui_.sample_table->getId(row);
		emit openProcessedSampleTab(db_.processedSampleName(ps_id));
	}
}

void SampleSearchWidget::openProcessedSample()
{
	QSet<int> rows = ui_.sample_table->selectedRows();
	if (rows.count()>1)
	{
		QMessageBox::warning(this, "Error opening processed sample", "Please select one sample!\nOnly one processed sample can be opened at a time.");
		return;
	}
	foreach(int row, rows)
	{
		QString ps_id = ui_.sample_table->getId(row);
		emit openProcessedSample(db_.processedSampleName(ps_id));
	}
}

void SampleSearchWidget::deleteSampleData()
{
	//get processed sample IDs
	QStringList ps_ids;
	QSet<int> rows = ui_.sample_table->selectedRows();
	foreach(int row, rows)
	{
		ps_ids << ui_.sample_table->getId(row);
	}

	//dialog
	ProcessedSampleDataDeletionDialog* dlg = new ProcessedSampleDataDeletionDialog(this, ps_ids);
	dlg->exec();
}

void SampleSearchWidget::queueAnalysis()
{
	//prepare sample list
	QList<AnalysisJobSample> samples;
	QSet<int> rows = ui_.sample_table->selectedRows();
	foreach(int row, rows)
	{
		QString ps_id = ui_.sample_table->getId(row);
		samples << AnalysisJobSample {db_.processedSampleName(ps_id), ""};
	}

	//show dialog
	SingleSampleAnalysisDialog dlg(this);
	dlg.setSamples(samples);
	if (dlg.exec()!=QDialog::Accepted) return;

	//start analysis
	foreach(const AnalysisJobSample& sample,  dlg.samples())
	{
		db_.queueAnalysis("single sample", dlg.highPriority(), dlg.arguments(), QList<AnalysisJobSample>() << sample);
	}
}

