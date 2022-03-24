#include "DBPermissionsEditor.h"
#include "DBComboBox.h"
#include "NGSD.h"
#include "UserPermissionList.h"
#include <QGridLayout>
#include <QMessageBox>

DBPermissionsEditor::DBPermissionsEditor(QString user_id, QWidget *parent) :
	QWidget(parent)	
	, ui_()
	, user_id_(user_id)
{
	ui_.setupUi(this);
	permission_types_ = getEnumList("user_permissions", "permission");

	QComboBox* permission_selector = new QComboBox(this);
	permission_selector->addItems(permission_types_);

	QWidget* permission_widget = permission_selector;
	permission_widget->setObjectName("permission_list");
	permission_widget->setEnabled(true);
	ui_.form_layout->addRow(tr("Permission"), permission_widget);
	connect(permission_widget, SIGNAL(textActivated(QString)), this, SLOT(permissionTextChanged(QString)));
	connect(permission_widget, SIGNAL(activated(int)), this, SLOT(permissionIndexChanged(int)));

	permission_selector->setCurrentIndex(0);
	emit(permission_selector->activated(0));
}

void DBPermissionsEditor::store()
{
	try
	{
		QString selected_data_value;
		switch(UserPermissionList::stringToType(selected_permission_))
		{
			case Permission::PROJECT:
				selected_data_value = db_.getValue("SELECT id FROM project WHERE name='"+selected_data_+"'").toString();
				break;
			case Permission::PROJECT_TYPE:
				selected_data_value = selected_data_;
				break;
			case Permission::STUDY:
				selected_data_value = db_.getValue("SELECT id FROM study WHERE name='"+selected_data_+"'").toString();
				break;
			case Permission::SAMPLE:
				selected_data_value = db_.getValue("SELECT id FROM sample WHERE name='"+selected_data_+"'").toString();
				break;
		}

		int same_permission_count = db_.getValue("SELECT count(id) FROM user_permissions WHERE user_id='" + user_id_ +"' AND permission='"+selected_permission_+"' AND data='"+selected_data_value+"'").toInt();
		if (same_permission_count>0)
		{
			QMessageBox::information(this, "Permission exists", "The selected permission already exists for the given user");
			return;
		}

		SqlQuery query = db_.getQuery();
		query.exec("INSERT INTO user_permissions (user_id, permission, data) VALUES ('" + user_id_ + "','" + selected_permission_ + "','" + selected_data_value + "')");
	}
	catch (DatabaseException& e)
	{
		QMessageBox::warning(this, "Adding new permission", "Error while adding a new permission to the database: " + e.message());
	}
}

void DBPermissionsEditor::permissionTextChanged(QString value)
{
	selected_permission_ = value;
	switch(UserPermissionList::stringToType(selected_permission_))
	{
		case Permission::PROJECT:
			data_values_ = db_.getValues("SELECT name FROM project");
			break;
		case Permission::PROJECT_TYPE:
			data_values_ = getEnumList("project", "type");
			break;
		case Permission::STUDY:
			data_values_ = db_.getValues("SELECT name FROM study");
			break;
		case Permission::SAMPLE:
			data_values_ = db_.getValues("SELECT name FROM sample");
			break;
	}

	createDataList(true);
}

void DBPermissionsEditor::permissionIndexChanged(int index)
{
	if (permission_types_.count() < index) return;
	permissionTextChanged(permission_types_[index]);
}

void DBPermissionsEditor::dataChanged(QString value)
{
	selected_data_ = value;
}

void DBPermissionsEditor::dataIndexChanged(int index)
{
	if (data_values_.count() < index) return;
	dataChanged(data_values_[index]);
}

QStringList DBPermissionsEditor::getEnumList(QString table_name, QString filed_name)
{
	const TableInfo& table_info = db_.tableInfo(table_name);
	foreach(const QString& field, table_info.fieldNames())
	{
		const TableFieldInfo& field_info = table_info.fieldInfo(field);
		if ((field_info.type==TableFieldInfo::ENUM) && (field.toLower()==filed_name.toLower()))
		{
			QStringList items = field_info.type_constraints.valid_strings;
			if (field_info.is_nullable) items.prepend("");
			return items;
		}
	}
	return QStringList();
}

void DBPermissionsEditor::createDataList(bool is_enabled)
{
	QComboBox* data_selector = new QComboBox(this);
	data_selector->addItems(data_values_);
	data_widget_ = data_selector;
	data_widget_->setObjectName("data_list");
	data_widget_->setEnabled(is_enabled);
	if (ui_.form_layout->rowCount()>=2)	ui_.form_layout->removeRow(ui_.form_layout->rowCount()-1);

	ui_.form_layout->addRow(tr("Entity"), data_widget_);
	connect(data_widget_, SIGNAL(textActivated(QString)), this, SLOT(dataChanged(QString)));
	connect(data_widget_, SIGNAL(activated(int)), this, SLOT(dataIndexChanged(int)));

	data_selector->setCurrentIndex(0);
	emit(data_selector->activated(0));
}

