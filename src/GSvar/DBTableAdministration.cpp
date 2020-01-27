#include "DBTableAdministration.h"
#include "NGSD.h"
#include "DBEditor.h"
#include "GUIHelper.h"
#include <QMessageBox>

DBTableAdministration::DBTableAdministration(QString table, QWidget* parent)
	: QWidget(parent)
	, ui_()
	, table_(table)
	, table_display_name_(table.replace("_", " "))
{
	ui_.setupUi(this);

	connect(ui_.add_btn, SIGNAL(clicked(bool)), this, SLOT(add()));
	connect(ui_.edit_btn, SIGNAL(clicked(bool)), this, SLOT(edit()));
	connect(ui_.delete_btn, SIGNAL(clicked(bool)), this, SLOT(remove()));
	connect(ui_.text_filter_btn, SIGNAL(clicked(bool)), this, SLOT(updateTable()));

	QAction* action = new QAction(QIcon(":/Icons/Edit.png"), "Edit", this);
	ui_.table->addAction(action);
	connect(action, SIGNAL(triggered(bool)), this, SLOT(edit()));

	action = new QAction(QIcon(":/Icons/Remove.png"), "Delete", this);
	ui_.table->addAction(action);
	connect(action, SIGNAL(triggered(bool)), this, SLOT(remove()));

	updateTable();
}

void DBTableAdministration::updateTable()
{
	NGSD db;
	DBTable db_table = db.createOverviewTable(table_, ui_.text_filter->text());
	ui_.table->setData(db_table);
}

void DBTableAdministration::add()
{
	//add
	DBEditor* editor = new DBEditor(this, table_);
	auto dlg = GUIHelper::createDialog(editor, "Add " + table_display_name_, "", true);
	if (dlg->exec()==QDialog::Accepted)
	{
		try
		{
			editor->store();
			updateTable();
		}
		catch (DatabaseException e)
		{
			QMessageBox::warning(this, "Error storing item", "Could not store the item.\n\nDatabase error:\n" + e.message());
		}
	}
}

void DBTableAdministration::edit()
{
	//check
	QSet<int> rows = ui_.table->selectedRows();
	if (rows.count()!=1)
	{
		QMessageBox::information(this, "Selection error", "Please select exactly one item!");
		return;
	}

	//edit
	int id = ui_.table->getId(rows.toList().first()).toInt();
	DBEditor* editor = new DBEditor(this, table_, id);
	auto dlg = GUIHelper::createDialog(editor, "Edit " + table_display_name_, "", true);
	if (dlg->exec()==QDialog::Accepted)
	{
		try
		{
			editor->store();
			updateTable();
		}
		catch (DatabaseException e)
		{
			QMessageBox::warning(this, "Error storing item", "Could not store the item.\n\nDatabase error:\n" + e.message());
		}
	}
}

void DBTableAdministration::remove()
{
	//check
	QSet<int> rows = ui_.table->selectedRows();
	if (rows.count()==0)
	{
		QMessageBox::information(this, "Selection error", "Please select at least one item!");
		return;
	}

	//confirm
	int btn = QMessageBox::information(this, "Confirm deleting", "You have selected " + QString::number(rows.count()) + " items.\nDo you really want to delete them?", QMessageBox::Yes, QMessageBox::Cancel);
	if (btn!=QMessageBox::Yes) return;

	//delete
	NGSD db;
	SqlQuery query = db.getQuery();
	try
	{
		foreach(int row, rows)
		{
			 query.exec("DELETE FROM " + table_ + " WHERE id=" + ui_.table->getId(row));
		}
	}
	catch (DatabaseException e)
	{
		QMessageBox::warning(this, "Error deleting item", "Could not delete an item!"
														  "\nThis is probably caused by the item being referenced from another table."
														  "\n\nDatabase error:\n" + e.message());
	}

	updateTable();
}

