#include "DBTablePermissions.h"
#include "UserPermissionList.h"
#include "DBEditor.h"
#include "GUIHelper.h"
#include "DBPermissionsEditor.h"
#include <QMessageBox>

DBTablePermissions::DBTablePermissions(QString table, QString user_id, QWidget* parent) :
	QWidget(parent)
	, ui_()
	, table_(table)
	, user_id_(user_id)
	, table_display_name_(table.replace("_", " "))
	, init_timer_(this, true)
{
	ui_.setupUi(this);

	connect(ui_.add_btn, SIGNAL(clicked(bool)), this, SLOT(add()));
	connect(ui_.delete_btn, SIGNAL(clicked(bool)), this, SLOT(remove()));
}

void DBTablePermissions::delayedInitialization()
{
	updateTable();
}

void DBTablePermissions::updateTable()
{
	QApplication::setOverrideCursor(Qt::BusyCursor);

	//create table
	NGSD db;
	DBTable db_table = db.createTable("user_permissions", "SELECT id, permission, data AS entity FROM " + table_ + " WHERE user_id='" + user_id_ + "'");
	DBTable db_empty_table;
	db_empty_table.setTableName("Permissions");
	db_empty_table.setHeaders(db_table.headers());

	// Replacing id numbers with human readable names for the entities permissions are assigned to
	for (int i = 0; i < db_table.rowCount(); i++)
	{
		QString human_readable_data = "unknown";
		QString permission_str = db_table.row(i).value(0);
		QString data_hint = db_table.row(i).value(1);
		DBRow new_row;

		switch(UserPermissionList::stringToType(permission_str))
		{
			case Permission::PROJECT:
				human_readable_data = db.getValue("SELECT name FROM project WHERE id='"+ data_hint +"'").toString();				
				break;
			case Permission::PROJECT_TYPE:
				human_readable_data = data_hint;
				break;
			case Permission::STUDY:
				human_readable_data = db.getValue("SELECT name FROM study WHERE id='"+ data_hint +"'").toString();
				break;
			case Permission::SAMPLE:
				human_readable_data = db.getValue("SELECT name FROM sample WHERE id='"+ data_hint +"'").toString();				
				break;
		}

		new_row.setId(db_table.row(i).id());
		new_row.addValue(db_table.row(i).value(0));
		new_row.addValue(human_readable_data);
		db_empty_table.addRow(new_row);
	}

	ui_.table->setData(db_empty_table);
	QApplication::restoreOverrideCursor();
}

void DBTablePermissions::add()
{
	//add
	DBPermissionsEditor* widget = new DBPermissionsEditor(user_id_, this);
	auto dlg = GUIHelper::createDialog(widget, "Adding a new permission", "Select a permission type and an item this permission is assigned to:", true);
	if (dlg->exec()==QDialog::Accepted)
	{
		widget->store();
		updateTable();
	}
}

void DBTablePermissions::remove()
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
