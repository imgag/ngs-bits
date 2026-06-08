#ifndef USERACCESSPERMISSIONLIST_H
#define USERACCESSPERMISSIONLIST_H

#include "cppNGSD_global.h"
#include "Exceptions.h"
#include <QString>

/// User permission items (used in user_permissions table)
enum class AccessPermission
{	
	PROJECT, // only a specific project
	PROJECT_TYPE, // only specific types of projects
	STUDY, // only a specific study
	SAMPLE, // only a specific sample
};

struct UserPermission
{
	AccessPermission permission;
	QString data;
};

class CPPNGSDSHARED_EXPORT UserPermissionList
	: public QList<UserPermission>
{

public:
	UserPermissionList()
	{
	}

	static AccessPermission stringToType(const QString& in)
	{
		if (in.toLower() == "project") {return AccessPermission::PROJECT;}
		if (in.toLower() == "project_type") {return AccessPermission::PROJECT_TYPE;}
		if (in.toLower() == "study") {return AccessPermission::STUDY;}
		if (in.toLower() == "sample") {return AccessPermission::SAMPLE;}

		THROW(ProgrammingException, "Unhandled permission type '" + in + "' in stringToType()!");
	}

	static QString typeToString(AccessPermission in)
	{
		switch(in)
		{
			case AccessPermission::PROJECT:
				return "PROJECT";
			case AccessPermission::PROJECT_TYPE:
				return "PROJECT_TYPE";
			case AccessPermission::STUDY:
				return "STUDY";
			case AccessPermission::SAMPLE:
				return "SAMPLE";			
		}
		THROW(ProgrammingException, "Unhandled permission type '" + QString::number((int)in) + "' in typeToString()!");
	}
};
#endif // USERACCESSPERMISSIONLIST_H
