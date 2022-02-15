#include "UserPermissionProvider.h"

UserPermissionProvider::UserPermissionProvider(int user_id)
	:user_id_(user_id)
{
	NGSD db;
	SqlQuery query = db.getQuery();
	query.exec("SELECT * FROM user_permissions WHERE user_id=" + QString::number(user_id_));
	if (query.size() == 0 ) THROW(DatabaseException, "User with id '" + QString::number(user_id) + "'not found in table 'user_permissions'!" );
	while(query.next())
	{
		UserPermission current_permission;
		current_permission.user_id = query.value("user_id").toInt();
		current_permission.permission = UserPermissionList::stringToType(query.value("permission").toString());
		current_permission.data = query.value("data").toString();
		user_permissions_.addPermission(current_permission);
	}
}

UserPermissionList UserPermissionProvider::getUserPermissions()
{
	return user_permissions_;
}

bool UserPermissionProvider::isEligibleToAccessProcessedSampleById(QString ps_id)
{
	NGSD db;
	for(int i = 0; i < user_permissions_.size(); i++)
	{
		switch(user_permissions_[i].permission)
		{
			case Permission::META_DATA:
				return false; // not implementent so far
			case Permission::PROJECT:
				return user_permissions_[i].data == db.getValue("SELECT project_id FROM processed_sample WHERE id='"+ ps_id +"'").toString();
			case Permission::PROJECT_TYPE:
				return user_permissions_[i].data.toLower() == db.getValue("SELECT type FROM project INNER JOIN processed_sample ON project.id=processed_sample.project_id WHERE processed_sample.id='"+ ps_id +"'").toString().toLower();
			case Permission::STUDY:
				return user_permissions_[i].data.toLower() == db.getValue("SELECT study_id FROM study_sample INNER JOIN processed_sample ON study_sample.processed_sample_id=processed_sample.id WHERE processed_sample.id='"+ ps_id +"'").toString().toLower();
			case Permission::SAMPLE:
				return user_permissions_[i].data.toLower() == db.getValue("SELECT sample_id FROM processed_sample INNER JOIN sample ON processed_sample.sample_id=sample.id WHERE processed_sample.id='"+ ps_id +"'").toString().toLower();
			case Permission::UNDEFINED:
				return false;
		}
	}
	return false;
}

bool UserPermissionProvider::isEligibleToAccessProcessedSampleByName(QString ps_name)
{
	NGSD db;
	QString ps_id = db.processedSampleId(ps_name);
	return isEligibleToAccessProcessedSampleById(ps_id);
}
