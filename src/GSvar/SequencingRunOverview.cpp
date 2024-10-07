#include "SequencingRunOverview.h"
#include "NGSD.h"
#include "DBEditor.h"
#include "GUIHelper.h"
#include "DBComboBox.h"
#include "GlobalServiceProvider.h"
#include <QMessageBox>
#include <QAction>

SequencingRunOverview::SequencingRunOverview(QWidget *parent)
	: QWidget(parent)
	, ui_()
	, init_timer_(this, true)
{
	ui_.setupUi(this);
	connect(ui_.text_filter, SIGNAL(returnPressed()), this, SLOT(updateTable()));
	connect(ui_.project_type, SIGNAL(currentIndexChanged(int)), this, SLOT(updateTable()));
	connect(ui_.update_btn, SIGNAL(clicked(bool)), this, SLOT(updateTable()));
	connect(ui_.add_btn, SIGNAL(clicked(bool)), this, SLOT(addRun()));
	connect(ui_.table, SIGNAL(rowDoubleClicked(int)), this, SLOT(openRunTab(int)));

	//project type filter
	ui_.project_type->addItem("");
	ui_.project_type->addItems(NGSD().getEnum("project", "type"));

	//table context menu
	QAction* action = new QAction(QIcon(":/Icons/NGSD_run.png"), "Open sequencing run tab");
	ui_.table->addAction(action);
	connect(action, SIGNAL(triggered(bool)), this, SLOT(openRunTab()));

	action = new QAction(QIcon(":/Icons/Edit.png"), "Edit sequencing run");
	ui_.table->addAction(action);
	connect(action, SIGNAL(triggered(bool)), this, SLOT(editRun()));

	action = new QAction(QIcon(":/Icons/Exchange.png"), "Move processed samples to other run");
	ui_.table->addAction(action);
	connect(action, SIGNAL(triggered(bool)), this, SLOT(moveSamples()));

	//batch view for lrGS samples
	action = new QAction(QIcon(":/Icons/NGSD_run_overview.png"), "Open sequencing run batch");
	ui_.table->addAction(action);
	connect(action, SIGNAL(triggered(bool)), this, SLOT(openRunBatchTab()));
}

void SequencingRunOverview::delayedInitialization()
{
	updateTable();
}

void SequencingRunOverview::updateTable()
{
	QApplication::setOverrideCursor(Qt::BusyCursor);

	//create table
	NGSD db;
	DBTable table = db.createOverviewTable("sequencing_run", ui_.text_filter->text(), "name DESC");

	//project type filter
	if (ui_.project_type->currentText()!="")
	{
		QStringList runs = db.getValues("SELECT DISTINCT r.name FROM sequencing_run r, processed_sample ps, project p WHERE r.id=ps.sequencing_run_id AND ps.project_id=p.id AND p.type='" + ui_.project_type->currentText() + "'");
		int c = table.columnIndex("name");
		table.filterRowsByColumn(c, runs);
	}

	//add sample count column (before status)
	QHash<QString, QString> counts;
	SqlQuery query = db.getQuery();
	query.exec("SELECT r.id, COUNT(ps.id) FROM processed_sample ps, sequencing_run r WHERE ps.sequencing_run_id=r.id GROUP BY r.id");
	while(query.next())
	{
		counts[query.value(0).toString()] = query.value(1).toString();
	}
	QStringList column;
	for (int r=0; r<table.rowCount(); ++r)
	{
		column << counts.value(table.row(r).id());
	}
	table.insertColumn(table.columnIndex("status"), column, "sample count");

	//mark runs where there is something to do with '...'
	int status_col = table.columnIndex("status");
	column = table.extractColumn(status_col);
	for (int i=0; i<column.count(); ++i)
	{
		if (column[i]=="run_started" || column[i]=="run_finished" ||column[i]=="demultiplexing_started" ||column[i]=="analysis_started")
		{
			column[i] = column[i] + " ...";
		}
	}
	table.setColumn(status_col, column);

	//show table (quality as icons)
	QStringList quality_values = table.takeColumn(table.columnIndex("quality"));
	ui_.table->setData(table);
	ui_.table->setQualityIcons("name", quality_values);

	//color
	QColor yellow = QColor(255,255,0,125);
	ui_.table->setBackgroundColorIfContains("status", yellow, "...");
	QColor red = QColor(255,0,0,125);
	ui_.table->setBackgroundColorIfEqual("status", red, "analysis_not_possible");
	ui_.table->setBackgroundColorIfEqual("status", red, "run_aborted");
	ui_.table->setBackgroundColorIfEqual("backup done", red, "no");

	QApplication::restoreOverrideCursor();
}

void SequencingRunOverview::openRunTab()
{
	//determine name column
	int col = ui_.table->columnIndex("name");

	//open tabs
	QSet<int> rows = ui_.table->selectedRows();
	foreach (int row, rows)
	{
		QString name = ui_.table->item(row, col)->text();
		GlobalServiceProvider::openRunTab(name);
	}
}

void SequencingRunOverview::openRunBatchTab()
{
	//determine name column
	int idx_name = ui_.table->columnIndex("name");
	int idx_device = ui_.table->columnIndex("device");

	//open a batch view with all runs
	QSet<int> rows = ui_.table->selectedRows();
	QStringList run_names;
	QSet<QString> device_types;
	foreach (int row, rows)
	{
		run_names << ui_.table->item(row, idx_name)->text();
		//extract device type
		device_types << ui_.table->item(row, idx_device)->text().split("(").at(1).split(")").at(0);
	}
	if (device_types.size() > 1)
	{
		QMessageBox::critical(this, "Batch run view", "Batch view is only supported for runs from the same sequencer type!");
		return;
	}

	GlobalServiceProvider::openRunBatchTab(run_names);
}

void SequencingRunOverview::openRunTab(int row)
{
	int col = ui_.table->columnIndex("name");
	QString name = ui_.table->item(row, col)->text();
	GlobalServiceProvider::openRunTab(name);
}

void SequencingRunOverview::editRun()
{
	//check
	QSet<int> rows = ui_.table->selectedRows();
	if (rows.count()!=1)
	{
		QMessageBox::critical(this, "Edit run", "Please select exactly one run!");
		return;
	}
	int row = rows.toList().first();

	//determine name column
	int col = ui_.table->columnIndex("name");

	//edit
	DBEditor* widget = new DBEditor(this, "sequencing_run", ui_.table->getId(row).toInt());
	auto dlg = GUIHelper::createDialog(widget, "Edit sequencing run " + ui_.table->item(row, col)->text() ,"", true);
	if (dlg->exec()==QDialog::Accepted)
	{
		widget->store();
		updateTable();
	}
}

void SequencingRunOverview::moveSamples()
{
	//check one run is selected
	QSet<int> rows = ui_.table->selectedRows();
	if (rows.count()!=1)
	{
		QMessageBox::critical(this, "Moving samples", "Please select exactly one run!");
		return;
	}
	int row = rows.toList().first();

	//check run status
	int status_col = ui_.table->columnIndex("status");
	QString status = ui_.table->item(row, status_col)->text();
	if (status!="run_aborted" && status!="analysis_not_possible")
	{
		QMessageBox::critical(this, "Moving samples", "Please select a run with status 'run_aborted' or 'analysis_not_possible'!");
		return;
	}

	//select target run
	NGSD db;
	DBComboBox* box = new DBComboBox(this);
	box->fill(db.createTable("sequencing_run", "SELECT id, name FROM sequencing_run ORDER BY id DESC"));
	auto dlg = GUIHelper::createDialog(box, "Select target run", "target run:", true);
	if (dlg->exec()!=QDialog::Accepted) return;
	QString target_run_id = box->getCurrentId();
	if (target_run_id=="") return;

	//move samples
	QString souce_run_id = ui_.table->getId(row);
	db.getQuery().exec("UPDATE processed_sample SET sequencing_run_id='" + target_run_id + "' WHERE sequencing_run_id='" + souce_run_id + "'");
	updateTable();
}

void SequencingRunOverview::addRun()
{
	DBEditor* widget = new DBEditor(this, "sequencing_run");
	auto dlg = GUIHelper::createDialog(widget, "Add sequencing run" ,"", true);
	if (dlg->exec()==QDialog::Accepted)
	{
		widget->store();
		updateTable();
	}
}
