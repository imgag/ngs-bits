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
};
#endif // USERPERMISSIONLIST_H
