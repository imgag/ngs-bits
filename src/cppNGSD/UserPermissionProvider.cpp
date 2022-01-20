#include "UserPermissionProvider.h"

UserPermissionProvider::UserPermissionProvider(int user_id)
	:user_id_(user_id)
{
	user_permissions_.setUserId(user_id_);
	NGSD db;
	QStringList permissions = db.getValues("SELECT permission FROM user_permissions WHERE user_id=" + QString::number(user_id_));

	for(int i = 0; i < permissions.length(); i++)
	{
		user_permissions_.addPermission(UserPermissions::stringToType(permissions[i]));
	}
}

UserPermissions UserPermissionProvider::getUserPermissions()
{
	return user_permissions_;
}
