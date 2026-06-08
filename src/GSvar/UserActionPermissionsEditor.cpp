#include "UserActionPermissionsEditor.h"
#include <QMessageBox>
#include "LoginManager.h"
#include "ClientHelper.h"

UserActionPermissionsEditor::UserActionPermissionsEditor(QString table, QString user_id, QWidget* parent) :
	QWidget(parent)
	, ui_()
	, table_(table)
	, user_id_(user_id)
	, table_display_name_(table.replace("_", " "))
{
	ui_.setupUi(this);

	connect(ui_.read_only_cb, SIGNAL(stateChanged(int)), this, SLOT(updateTable()));
	connect(ui_.variant_search_cb, SIGNAL(stateChanged(int)), this, SLOT(updateTable()));
	connect(ui_.burden_test_cb, SIGNAL(stateChanged(int)), this, SLOT(updateTable()));
	connect(ui_.start_analysis_jobs_cb, SIGNAL(stateChanged(int)), this, SLOT(updateTable()));	

	NGSD db;
	SqlQuery query = db.getQuery();
	query.exec("SELECT read_only, perform_variant_search, perform_burden_test, start_analysis_jobs FROM user_action_permissions WHERE user_id='" + user_id_ + "'");
	while(query.next())
	{
		if (query.value(0).toBool()) ui_.read_only_cb->setChecked(true);
		if (query.value(1).toBool()) ui_.variant_search_cb->setChecked(true);
		if (query.value(2).toBool()) ui_.burden_test_cb->setChecked(true);
		if (query.value(3).toBool()) ui_.start_analysis_jobs_cb->setChecked(true);		
	}
}

void UserActionPermissionsEditor::updateTable()
{
	NGSD db;

	try
	{
		// remove the record, if none of the checkboxes is unchecked
		if (!ui_.read_only_cb->isChecked() && !ui_.variant_search_cb->isChecked() && !ui_.burden_test_cb->isChecked() && !ui_.start_analysis_jobs_cb->isChecked())
		{
			db.getQuery().exec("DELETE FROM " + table_ + " WHERE user_id=" + user_id_);
			return;
		}

		if (db.getValue("SELECT count(id) FROM user_action_permissions WHERE user_id='" + user_id_ + "'").toInt()==0)
		{
			db.getQuery().exec("INSERT INTO user_action_permissions (user_id, read_only, perform_variant_search, perform_burden_test, start_analysis_jobs) VALUES ('" + user_id_ + "','"+QString::number(ui_.read_only_cb->isChecked()) + "','"+QString::number(ui_.variant_search_cb->isChecked())+"', '"+QString::number(ui_.burden_test_cb->isChecked())+"', '"+QString::number(ui_.start_analysis_jobs_cb->isChecked())+"')");
		}
		else
		{
			db.getQuery().exec("UPDATE user_action_permissions SET read_only='"+QString::number(ui_.read_only_cb->isChecked())+"', perform_variant_search='"+QString::number(ui_.variant_search_cb->isChecked())+"', perform_burden_test='"+QString::number(ui_.burden_test_cb->isChecked())+"', start_analysis_jobs='"+QString::number(ui_.start_analysis_jobs_cb->isChecked())+"' WHERE user_id='"+user_id_+ "'");
		}
	}
	catch (DatabaseException e)
	{
		QMessageBox::warning(this, "Error while changing permissions", "Could not save action permissions for the selected user: " + e.message());
	}
	clearServerCache();
}

void UserActionPermissionsEditor::clearServerCache()
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
