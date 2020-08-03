#include "SequencingRunOverview.h"
#include "NGSD.h"
#include "DBEditor.h"
#include "GUIHelper.h"
#include <QMessageBox>
#include  <QAction>

SequencingRunOverview::SequencingRunOverview(QWidget *parent)
	: QWidget(parent)
	, ui_()
	, init_timer_(this, true)
{
	ui_.setupUi(this);
	connect(ui_.text_filter, SIGNAL(editingFinished()), this, SLOT(updateTable()));
	connect(ui_.text_filter_btn, SIGNAL(clicked(bool)), this, SLOT(updateTable()));
	connect(ui_.add_btn, SIGNAL(clicked(bool)), this, SLOT(addRun()));
	connect(ui_.table, SIGNAL(rowDoubleClicked(int)), this, SLOT(openRunTab(int)));

	//table context menu
	QAction* action = new QAction(QIcon(":/Icons/NGSD_run.png"), "Open sequencing run tab");
	ui_.table->addAction(action);
	connect(action, SIGNAL(triggered(bool)), this, SLOT(openRunTab()));
	action = new QAction(QIcon(":/Icons/Edit.png"), "Edit sequencing run");
	ui_.table->addAction(action);
	connect(action, SIGNAL(triggered(bool)), this, SLOT(editRun()));
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
	QColor red = QColor(255,0,0,125);
	ui_.table->setBackgroundColorIfEqual("status", red, "analysis_not_possible");
	ui_.table->setBackgroundColorIfEqual("status", red, "run_aborted");
	ui_.table->setBackgroundColorIfEqual("backup done", red, "no");

	QApplication::restoreOverrideCursor();
}

void SequencingRunOverview::openRunTab()
{
	//determine name column
	int name_col = ui_.table->columnIndex("name");

	//open tabs
	QSet<int> rows = ui_.table->selectedRows();
	foreach (int row, rows)
	{
		emit openRun(ui_.table->item(row, name_col)->text());
	}
}

void SequencingRunOverview::openRunTab(int row)
{
	int name_col = ui_.table->columnIndex("name");

	emit openRun(ui_.table->item(row, name_col)->text());
}

void SequencingRunOverview::editRun()
{
	//check
	QSet<int> rows = ui_.table->selectedRows();
	if (rows.count()!=1)
	{
		QMessageBox::information(this, "Selection error", "Please select exactly one item to edit!");
		return;
	}
	int row = rows.toList().first();

	//determine name column
	int name_col = ui_.table->columnIndex("name");

	//edit
	DBEditor* widget = new DBEditor(this, "sequencing_run", ui_.table->getId(row).toInt());
	auto dlg = GUIHelper::createDialog(widget, "Edit sequencing run " + ui_.table->item(row, name_col)->text() ,"", true);
	if (dlg->exec()==QDialog::Accepted)
	{
		widget->store();
		updateTable();
	}
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
