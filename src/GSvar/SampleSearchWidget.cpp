#include "SampleSearchWidget.h"
#include "NGSD.h"
#include <QMessageBox>

SampleSearchWidget::SampleSearchWidget(QWidget* parent)
	: QWidget(parent)
	, ui_()
	, db_()
{
	ui_.setupUi(this);

	//context menu
	QAction* action = new QAction(QIcon(":/Icons/NGSD_sample.png"), "Open processed sample tab", this);
	ui_.sample_table->addAction(action);
	connect(action, SIGNAL(triggered(bool)), this, SLOT(openProcessedSample()));

	//init search criteria
	//sample
	ui_.s_name->fill(db_.createTable("sample", "SELECT id, name FROM sample"), true);
	ui_.s_species->fill(db_.createTable("species", "SELECT id, name FROM species"), true);
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

	params.add_outcome = ui_.add_outcome->isChecked();
	params.add_disease_details = ui_.add_disease_details->isChecked();
	params.add_qc = ui_.add_qc->isChecked();

	//execute query
	try
	{
		DBTable ps_table = db_.processedSampleSearch(params);
		ui_.sample_table->setData(ps_table);

		//text
		ui_.search_status->setText("Found " + QString::number(ps_table.rowCount()) + " matching samples.");
	}
	catch(DatabaseException& e)
	{
		QMessageBox::warning(this, "NGSD error", "Database error:\n" + e.message());
	}

	QApplication::restoreOverrideCursor();
}

void SampleSearchWidget::openProcessedSample()
{

	QSet<int> rows = ui_.sample_table->selectedRows();
	foreach(int row, rows)
	{
		QString ps_id = ui_.sample_table->getId(row);
		emit openProcessedSampleTab(db_.processedSampleName(ps_id));
	}
}

