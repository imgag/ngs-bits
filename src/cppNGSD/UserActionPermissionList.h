#ifndef USERACTIONPERMISSIONLIST_H
#define USERACTIONPERMISSIONLIST_H

#include "cppNGSD_global.h"
#include "Exceptions.h"
#include <QString>

/// User action permission items (used in user_action_permissions table)
enum class ActionPermission
{
	READ_ONLY, // no report config, no NGSD modifications
	PERFORM_VARIANT_SEARCH, // ability to perform variant search
	PERFORM_BURDEN_TEST, // ability to perform burden test
	START_ANALYSIS_JOBS, // ability to start analysis jobs
};


class CPPNGSDSHARED_EXPORT UserActionPermissionList
	: public QList<ActionPermission>
{

public:
	UserActionPermissionList()
	{
	}

	static ActionPermission stringToType(const QString& in)
	{
		if (in.toLower() == "read_only") {return ActionPermission::READ_ONLY;}
		if (in.toLower() == "perform_variant_search") {return ActionPermission::PERFORM_VARIANT_SEARCH;}
		if (in.toLower() == "perform_burden_test") {return ActionPermission::PERFORM_BURDEN_TEST;}
		if (in.toLower() == "start_analysis_jobs") {return ActionPermission::START_ANALYSIS_JOBS;}

		THROW(ProgrammingException, "Unhandled permission type '" + in + "' in stringToType()!");
	}

	static QString typeToString(ActionPermission in)
	{
		switch(in)
		{
			case ActionPermission::READ_ONLY:
				return "READY_ONLY";
			case ActionPermission::PERFORM_VARIANT_SEARCH:
				return "PERFORM_VARIANT_SEARCH";
			case ActionPermission::PERFORM_BURDEN_TEST:
				return "PERFORM_BURDEN_TEST";
			case ActionPermission::START_ANALYSIS_JOBS:
				return "START_ANALYSIS_JOBS";
		}
		THROW(ProgrammingException, "Unhandled permission type '" + QString::number((int)in) + "' in typeToString()!");
	}
};
#endif // USERACTIONPERMISSIONLIST_H
