#ifndef USERPERMISSIONPROVIDER_H
#define USERPERMISSIONPROVIDER_H

#include "cppNGSD_global.h"
#include "UserPermissions.h"
#include "NGSD.h"


class CPPNGSDSHARED_EXPORT UserPermissionProvider
{
public:
	UserPermissionProvider(int user_id);

	///Returns all permissions for a predefined user
	UserPermissions getUserPermissions();

private:
	int user_id_;
	UserPermissions user_permissions_;

};

#endif // USERPERMISSIONPROVIDER_H
