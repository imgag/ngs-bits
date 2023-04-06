#include "UserPermissionsEditor.h"
#include "UserPermissionList.h"
#include "DBEditor.h"
#include "GUIHelper.h"
#include <QMessageBox>
#include <QMenu>
#include "DBSelector.h"

UserPermissionsEditor::UserPermissionsEditor(QString table, QString user_id, QWidget* parent) :
	QWidget(parent)
	, ui_()
	, table_(table)
	, user_id_(user_id)
	, table_display_name_(table.replace("_", " "))
	, init_timer_(this, true)
{
	ui_.setupUi(this);

	ui_.add_btn->setToolTip("Add a new user permission");
	ui_.add_btn->setMenu(new QMenu());
	ui_.add_btn->menu()->addSeparator();
	ui_.add_btn->menu()->addAction("Add project permission", this, SLOT(addProjectPermission()));
	ui_.add_btn->menu()->addAction("Add project type permission", this, SLOT(addProjectTypePermission()));
	ui_.add_btn->menu()->addAction("Add study permission", this, SLOT(addStudyPermission()));
	ui_.add_btn->menu()->addAction("Add sample permission", this, SLOT(addSamplePermission()));
	ui_.add_btn->setPopupMode(QToolButton::InstantPopup);

	connect(ui_.delete_btn, SIGNAL(clicked(bool)), this, SLOT(remove()));
}

void UserPermissionsEditor::delayedInitialization()
{
	updateTable();
}

void UserPermissionsEditor::updateTable()
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
				human_readable_data = db.sampleName(data_hint);
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

void UserPermissionsEditor::addProjectPermission()
{	
	createAddPermissionDialog("project");
}

void UserPermissionsEditor::addProjectTypePermission()
{
	QStringList enum_project_types;
	const TableInfo& table_info = db_.tableInfo("project");
	foreach(const QString& field, table_info.fieldNames())
	{
		const TableFieldInfo& field_info = table_info.fieldInfo(field);
		if ((field_info.type==TableFieldInfo::ENUM) && (field.toLower()=="type"))
		{
			QStringList items = field_info.type_constraints.valid_strings;
			if (field_info.is_nullable) items.prepend("");
			enum_project_types = items;
		}
	}

	DBTable output;
	output.setTableName("project_type");
	output.setHeaders({"type"});

	for (int i = 0; i < enum_project_types.count(); ++i)
	{
		DBRow row;
		row.setId(enum_project_types[i]);
		row.addValue(enum_project_types[i]);
		output.addRow(row);
	}

	DBSelector* selector = new DBSelector(this);
	selector->fill(output, true);

	QSharedPointer<QDialog> dialog = GUIHelper::createDialog(selector, "Select a project type", "Project type:", true);
	if (dialog->exec()==QDialog::Accepted && selector->isValidSelection())
	{
		addPermissionToDatabase("project_type", selector->getId(), selector->text());		
	}
}

void UserPermissionsEditor::addStudyPermission()
{
	createAddPermissionDialog("study");
}

void UserPermissionsEditor::addSamplePermission()
{
	createAddPermissionDialog("sample");
}

void UserPermissionsEditor::remove()
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
	SqlQuery query = db_.getQuery();
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

void UserPermissionsEditor::createAddPermissionDialog(QString table_name)
{
	QString entity_name = table_name;
	entity_name =  entity_name.replace(0, 1, entity_name[0].toUpper());
	DBSelector* selector = new DBSelector(this);
	selector->fill(db_.createTable(table_name, "SELECT id, name FROM " + table_name), true);

	QSharedPointer<QDialog> dialog = GUIHelper::createDialog(selector, "Select a " + entity_name.toLower(), entity_name + " name:", true);
	if (dialog->exec()==QDialog::Accepted && selector->isValidSelection())
	{
		addPermissionToDatabase(table_name, selector->getId(), selector->text());
	}
}

void UserPermissionsEditor::addPermissionToDatabase(QString permission, QString data, QString display_text)
{
	try
	{
		if (db_.getValue("SELECT count(id) FROM user_permissions WHERE user_id='" + user_id_ +"' AND permission='"+permission+"' AND data='"+data+"'").toInt() > 0)
		{
			QMessageBox::information(this, "Permission exists", "The permission '"+permission+"' already exists for '"+display_text+"'");
			return;
		}

		db_.getQuery().exec("INSERT INTO user_permissions (user_id, permission, data) VALUES ('" + user_id_ + "','" + permission + "','" + data + "')");
	}
	catch (DatabaseException& e)
	{
		QMessageBox::warning(this, "Adding new permission", "Error while adding a new permission to the database: " + e.message());
	}
	updateTable();
}
