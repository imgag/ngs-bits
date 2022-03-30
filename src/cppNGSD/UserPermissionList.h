#ifndef USERPERMISSIONLIST_H
#define USERPERMISSIONLIST_H

#include "cppNGSD_global.h"
#include "Exceptions.h"
#include <QString>

/// User permission items (used in user_permissions table)
enum class Permission
{
	PROJECT, // only a specific project
	PROJECT_TYPE, // only specific types of projects
	STUDY, // only a specific study
	SAMPLE // only a specific sample
};

struct UserPermission
{
	Permission permission;
	QString data;
};

class CPPNGSDSHARED_EXPORT UserPermissionList
	: public QList<UserPermission>
{

public:
	UserPermissionList()
	{
	}

	UserPermissionList(UserPermission permission)
	{
		item_list_.append(permission);
	}

	UserPermissionList(QList<UserPermission> permissions)
	{
		setPermissions(permissions);
	}

	void setUserId(int user_id)
	{
		user_id_ = user_id;
	}

	int getUserId()
	{
		return user_id_;
	}

	void setPermissions(QList<UserPermission> permissions)
	{
		item_list_ = permissions;
	}

	QList<UserPermission> getPermissions()
	{
		return item_list_;
	}

	void addPermission(UserPermission permission)
	{
		item_list_.append(permission);
	}

	void removeAllPermissions()
	{
		item_list_.clear();
	}

	static Permission stringToType(const QString& in)
	{
		if (in.toLower() == "project") {return Permission::PROJECT;}
		if (in.toLower() == "project_type") {return Permission::PROJECT_TYPE;}
		if (in.toLower() == "study") {return Permission::STUDY;}
		if (in.toLower() == "sample") {return Permission::SAMPLE;}

		THROW(ProgrammingException, "Unhandled permission type '" + in + "' in stringToType()!");
	}

	static QString typeToString(Permission in)
	{
		switch(in)
		{
			case Permission::PROJECT:
				return "PROJECT";
			case Permission::PROJECT_TYPE:
				return "PROJECT_TYPE";
			case Permission::STUDY:
				return "STUDY";
			case Permission::SAMPLE:
				return "SAMPLE";
		}
		THROW(ProgrammingException, "Unhandled permission type '" + QString::number((int)in) + "' in typeToString()!");
	}

private:
	QList<UserPermission> item_list_;
	int user_id_;
};
#endif // USERPERMISSIONLIST_H
