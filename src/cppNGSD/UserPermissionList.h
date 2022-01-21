#ifndef USERPERMISSIONLIST_H
#define USERPERMISSIONLIST_H

#include "cppNGSD_global.h"
#include "Exceptions.h"
#include <QString>

/// User permission items (used in user_permissions table)
enum class Permission
{
	META_DATA, // all metadata
	PROJECT, // only a specific project
	PROJECT_TYPE, // only specific types of projects
	STUDY, // only a specific study
	SAMPLE, // only a specific sample
	UNDEFINED // no persmissions available
};

struct UserPermission
{
	int user_id;
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
		if (in.toLower() == "meta_data") {return Permission::META_DATA;}
		if (in.toLower() == "project") {return Permission::PROJECT;}
		if (in.toLower() == "project_type") {return Permission::PROJECT_TYPE;}
		if (in.toLower() == "study") {return Permission::STUDY;}
		if (in.toLower() == "sample") {return Permission::SAMPLE;}

		return Permission::UNDEFINED;
	}

	static QString typeToString(Permission in)
	{
		switch(in)
		{
			case Permission::META_DATA:
				return "META_DATA";
			case Permission::PROJECT:
				return "PROJECT";
			case Permission::PROJECT_TYPE:
				return "PROJECT_TYPE";
			case Permission::STUDY:
				return "STUDY";
			case Permission::SAMPLE:
				return "SAMPLE";
			case Permission::UNDEFINED:
				return "UNDEFINED";
		}
		THROW(ProgrammingException, "Unhandled path type '" + QString::number((int)in) + "' in typeToString()!");
	}

private:
	QList<UserPermission> item_list_;
};
#endif // USERPERMISSIONLIST_H
