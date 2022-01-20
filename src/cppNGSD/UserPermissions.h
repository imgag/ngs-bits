#ifndef USERPERMISSIONS_H
#define USERPERMISSIONS_H

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

struct UserPermissions
{
	int user_id;
	QList<Permission> item_list;

	UserPermissions()
	{
	}

	UserPermissions(int user_id_)
		:user_id(user_id_)
	{
	}

	UserPermissions(int user_id_, Permission permission)
		:user_id(user_id_)
	{
		item_list.append(permission);
	}

	UserPermissions(int user_id_, QList<Permission> permissions)
		:user_id(user_id_)
	{
		item_list = permissions;
	}

	void setUserId(int user_id_)
	{
		user_id = user_id_;
	}

	int getUserId()
	{
		return user_id;
	}

	void setPermissions(QList<Permission> permissions)
	{
		item_list = permissions;
	}

	QList<Permission> getPermissions()
	{
		return item_list;
	}

	void addPermission(Permission permissions)
	{
		item_list.append(permissions);
	}

	void removeAllPermissions()
	{
		item_list.clear();
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

//	QString typeAsString() const
//	{
//		return typeToString(item);
//	}
};
#endif // USERPERMISSIONS_H
