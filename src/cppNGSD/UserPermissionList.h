#ifndef USERPERMISSIONLIST_H
#define USERPERMISSIONLIST_H

#include "cppNGSD_global.h"
#include "Exceptions.h"
#include <QString>

/// User permission items (used in user_permissions table)
enum class Permission
{
	// Data access permissions
	PROJECT, // only a specific project
	PROJECT_TYPE, // only specific types of projects
	STUDY, // only a specific study
	SAMPLE, // only a specific sample

	// Action permissions
	READ_ONLY, // no report config, no NGSD modifications
	PERFORM_VARIANT_SEARCH, // ability to perform variant search
	PERFORM_BURDEN_TEST, // ability to perform burden test
	START_ANALYSIS_JOBS, // ability to start analysis jobs
	PERFORM_SAMPLE_SEARCH // ability to run sample search
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

		if (in.toLower() == "read_only") {return Permission::READ_ONLY;}
		if (in.toLower() == "perform_variant_search") {return Permission::PERFORM_VARIANT_SEARCH;}
		if (in.toLower() == "perform_burden_test") {return Permission::PERFORM_BURDEN_TEST;}
		if (in.toLower() == "start_analysis_jobs") {return Permission::START_ANALYSIS_JOBS;}
		if (in.toLower() == "perform sample_search") {return Permission::PERFORM_SAMPLE_SEARCH;}

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

			case Permission::READ_ONLY:
				return "READY_ONLY";
			case Permission::PERFORM_VARIANT_SEARCH:
				return "PERFORM_VARIANT_SEARCH";
			case Permission::PERFORM_BURDEN_TEST:
				return "PERFORM_BURDEN_TEST";
			case Permission::START_ANALYSIS_JOBS:
				return "START_ANALYSIS_JOBS";
			case Permission::PERFORM_SAMPLE_SEARCH:
				return "PERFORM_SAMPLE_SEARCH";
		}
		THROW(ProgrammingException, "Unhandled permission type '" + QString::number((int)in) + "' in typeToString()!");
	}
};
#endif // USERPERMISSIONLIST_H
