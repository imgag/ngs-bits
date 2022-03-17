#ifndef LOGINMANAGER_H
#define LOGINMANAGER_H

#include <QString>
#include "cppNGSD_global.h"

///NGSD login manager (singleton)
class CPPNGSDSHARED_EXPORT LoginManager
{
public:
	//Returns user login from NGSD
	static QString user();
	//Returns the full user name from NGSD
	static QString userName();
	//Returns user ID from NGSD
	static int userId();
	//Returns user ID from NGSD (as string)
	static QString userIdAsString();

	//Returns a secure token generated for the given user
	static QString token();

	//Returns user role from NGSD
	static QString role();
	//Returns if a user is logged in (and that the NGSD is enabled)
	static bool active();

	//User is logged in
	static void login(QString user, QString token = "", bool test_db = false);
	//Log out user
	static void logout();

	//Checks if the user role is in the given role list. If not, an exception is thrown.
	static void checkRoleIn(QStringList roles);

private:
	LoginManager();
	static LoginManager& instance();

	QString user_;
	QString user_name_;
	int user_id_;
	QString role_;
	QString token_;
};

#endif // LOGINMANAGER_H
