#include "LoginManager.h"
#include "NGSD.h"

LoginManager::LoginManager()
	: user_()
	, role_()
{
}

LoginManager& LoginManager::instance()
{
	static LoginManager manager;
	return manager;
}

QString LoginManager::user()
{
	return instance().user_;
}

QString LoginManager::role()
{
	return instance().role_;
}

bool LoginManager::active()
{
	return !instance().user_.isEmpty();
}

void LoginManager::login(QString user)
{
	NGSD db;
	QString user_id = QString::number(db.userId(user, true));

	//determine role
	LoginManager& manager = instance();
	manager.user_ = user;
	manager.role_ = db.getValue("SELECT user_role FROM user WHERE id='" + user_id + "'").toString();

	//update last login
	db.getQuery().exec("UPDATE user SET last_login=NOW() WHERE id='" + user_id + "'");
}

void LoginManager::logout()
{
	LoginManager& manager = instance();
	manager.user_.clear();
	manager.role_.clear();
}

void LoginManager::checkRoleIn(QStringList roles)
{
	//check that role list is valid
	NGSD db;
	QStringList valid_roles = db.getEnum("user", "user_role");
	foreach(QString role, roles)
	{
		if (!valid_roles.contains(role)) THROW (ProgrammingException, "Invalid role '" + role + "' in LoginManager!");
	}

	//check if user has role
	LoginManager& manager = instance();
	if (!roles.contains(manager.role_))
	{
		THROW(Exception, "Access denied.\nOnly users with the roles '" + roles.join("', '") + "' have access.\nThe user '" + manager.user_ + "' has the role '" + manager.role_ + "'!");
		return;
	}
}
