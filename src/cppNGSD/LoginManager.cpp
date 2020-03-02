#include "LoginManager.h"
#include "NGSD.h"

LoginManager::LoginManager()
	: user_()
	, user_id_(-1)
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

int LoginManager::userId()
{
	int id = instance().user_id_;
	if (id==-1) THROW(ProgrammingException, "Cannot use LoginManager::userId() if no user is logged in!");
	return id;
}

QString LoginManager::userIdAsString()
{
	return QString::number(userId());
}

QString LoginManager::role()
{
	return instance().role_;
}

bool LoginManager::active()
{
	return !instance().user_.isEmpty();
}

void LoginManager::login(QString user, bool test_db)
{
	NGSD db(test_db);
	QString user_id = QString::number(db.userId(user, true));

	//determine role
	LoginManager& manager = instance();
	manager.user_ = user;
	manager.user_id_ = user_id.toInt();
	manager.role_ = db.getValue("SELECT user_role FROM user WHERE id='" + user_id + "'").toString();

	//update last login
	db.getQuery().exec("UPDATE user SET last_login=NOW() WHERE id='" + user_id + "'");
}

void LoginManager::logout()
{
	LoginManager& manager = instance();
	manager.user_.clear();
	manager.user_id_ = -1;
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
