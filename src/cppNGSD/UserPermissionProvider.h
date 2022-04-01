#ifndef USERPERMISSIONPROVIDER_H
#define USERPERMISSIONPROVIDER_H

#include "cppNGSD_global.h"
#include "UserPermissionList.h"
#include "NGSD.h"

class CPPNGSDSHARED_EXPORT UserPermissionProvider
{
public:
	UserPermissionProvider(int user_id);

	///Returns all permissions for a predefined user
	UserPermissionList getUserPermissions();

	bool isEligibleToAccessProcessedSampleById(QString ps_id);
	bool isEligibleToAccessProcessedSampleByName(QString ps_name);

private:
	int user_id_;
	UserPermissionList user_permissions_;

};

#endif // USERPERMISSIONPROVIDER_H
