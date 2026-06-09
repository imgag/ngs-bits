#include "UserPermissionsEditor.h"
#include "UserAccessPermissionList.h"
#include "GUIHelper.h"
#include <QMessageBox>
#include <QMenu>
#include "DBSelector.h"
#include "LoginManager.h"
#include "ClientHelper.h"

UserPermissionsEditor::UserPermissionsEditor(QString table, QString user_id, QWidget* parent) :
	QWidget(parent)
	, ui_()
	, table_(table)
	, user_id_(user_id)
	, table_display_name_(table.replace("_", " "))
	, init_timer_(this, true)
{
	ui_.setupUi(this);

	// initialize access permissions
	ui_.add_btn->setToolTip("Add a new user permission");
	ui_.add_btn->setMenu(new QMenu());
	ui_.add_btn->menu()->addSeparator();
	ui_.add_btn->menu()->addAction("Add project permission", this, SLOT(addProjectAccessPermission()));
	ui_.add_btn->menu()->addAction("Add project type permission", this, SLOT(addProjectTypeAccessPermission()));
	ui_.add_btn->menu()->addAction("Add study permission", this, SLOT(addStudyAccessPermission()));
	ui_.add_btn->menu()->addAction("Add sample permission", this, SLOT(addSampleAccessPermission()));
	ui_.add_btn->setPopupMode(QToolButton::InstantPopup);

	connect(ui_.delete_btn, SIGNAL(clicked(bool)), this, SLOT(removeAccessPermission()));

	// initialize action permissions
	connect(ui_.cb_read_only, SIGNAL(stateChanged(int)), this, SLOT(updateActionPermissions()));
	connect(ui_.cb_variant_search, SIGNAL(stateChanged(int)), this, SLOT(updateActionPermissions()));
	connect(ui_.cb_burden_test, SIGNAL(stateChanged(int)), this, SLOT(updateActionPermissions()));
	connect(ui_.cb_start_job, SIGNAL(stateChanged(int)), this, SLOT(updateActionPermissions()));

	NGSD db;
	SqlQuery query = db.getQuery();
	query.exec("SELECT read_only, perform_variant_search, perform_burden_test, start_analysis_jobs FROM user_action_permissions WHERE user_id='" + user_id_ + "'");
	while(query.next())
	{
		if (query.value(0).toBool()) ui_.cb_read_only->setChecked(true);
		if (query.value(1).toBool()) ui_.cb_variant_search->setChecked(true);
		if (query.value(2).toBool()) ui_.cb_burden_test->setChecked(true);
		if (query.value(3).toBool()) ui_.cb_start_job->setChecked(true);
	}
}

void UserPermissionsEditor::delayedInitialization()
{
	updateAccessPermissions();
}

void UserPermissionsEditor::updateAccessPermissions()
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
			case AccessPermission::PROJECT:
				human_readable_data = db.getValue("SELECT name FROM project WHERE id='"+ data_hint +"'").toString();				
				break;
			case AccessPermission::PROJECT_TYPE:
				human_readable_data = data_hint;
				break;
			case AccessPermission::STUDY:
				human_readable_data = db.getValue("SELECT name FROM study WHERE id='"+ data_hint +"'").toString();
				break;
			case AccessPermission::SAMPLE:
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

void UserPermissionsEditor::addProjectAccessPermission()
{	
	createAddAccessPermissionDialog("project");
}

void UserPermissionsEditor::addProjectTypeAccessPermission()
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
		addAccessPermissionToDatabase("project_type", selector->getId(), selector->text());
	}
}

void UserPermissionsEditor::addStudyAccessPermission()
{
	createAddAccessPermissionDialog("study");
}

void UserPermissionsEditor::addSampleAccessPermission()
{
	createAddAccessPermissionDialog("sample");
}

void UserPermissionsEditor::removeAccessPermission()
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

	updateAccessPermissions();
	clearServerCache();
}

void UserPermissionsEditor::updateActionPermissions()
{
	NGSD db;

	try
	{
		// remove the record, if none of the checkboxes is unchecked
		if (!ui_.cb_read_only->isChecked() && !ui_.cb_variant_search->isChecked() && !ui_.cb_burden_test->isChecked() && !ui_.cb_start_job->isChecked())
		{
			db.getQuery().exec("DELETE FROM user_action_permissions WHERE user_id=" + user_id_);
			return;
		}

		if (db.getValue("SELECT count(id) FROM user_action_permissions WHERE user_id='" + user_id_ + "'").toInt()==0)
		{
			db.getQuery().exec("INSERT INTO user_action_permissions (user_id, read_only, perform_variant_search, perform_burden_test, start_analysis_jobs) VALUES ('" + user_id_ + "','"+QString::number(ui_.cb_read_only->isChecked()) + "','"+QString::number(ui_.cb_variant_search->isChecked())+"', '"+QString::number(ui_.cb_burden_test->isChecked())+"', '"+QString::number(ui_.cb_start_job->isChecked())+"')");
		}
		else
		{
			db.getQuery().exec("UPDATE user_action_permissions SET read_only='"+QString::number(ui_.cb_read_only->isChecked())+"', perform_variant_search='"+QString::number(ui_.cb_variant_search->isChecked())+"', perform_burden_test='"+QString::number(ui_.cb_burden_test->isChecked())+"', start_analysis_jobs='"+QString::number(ui_.cb_start_job->isChecked())+"' WHERE user_id='"+user_id_+ "'");
		}
	}
	catch (DatabaseException e)
	{
		QMessageBox::warning(this, "Error while changing permissions", "Could not save action permissions for the selected user: " + e.message());
	}
	clearServerCache();
}

void UserPermissionsEditor::clearServerCache()
{
	try
	{
		HttpHeaders add_headers;
		add_headers.insert("Content-Type", "application/json");
		HttpRequestHandler().post(ClientHelper::serverApiUrl() + "clear_cache?token=" + LoginManager::userToken(), QByteArray{}, add_headers);
	}
	catch (Exception& e)
	{
		QMessageBox::warning(this, "Failed to clear user permissions cache on the server", e.message());
	}
}

void UserPermissionsEditor::createAddAccessPermissionDialog(QString table_name)
{
	QString entity_name = table_name;
	entity_name =  entity_name.replace(0, 1, entity_name[0].toUpper());
	DBSelector* selector = new DBSelector(this);
	selector->fill(db_.createTable(table_name, "SELECT id, name FROM " + table_name), true);

	QSharedPointer<QDialog> dialog = GUIHelper::createDialog(selector, "Select a " + entity_name.toLower(), entity_name + " name:", true);
	if (dialog->exec()==QDialog::Accepted && selector->isValidSelection())
	{
		addAccessPermissionToDatabase(table_name, selector->getId(), selector->text());
	}
}

void UserPermissionsEditor::addAccessPermissionToDatabase(QString permission, QString data, QString display_text)
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
	updateAccessPermissions();
	clearServerCache();
}
