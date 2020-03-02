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
	//Returns user identifier from NGSD
	static int userId();
	//Returns user identifier from NGSD (as string)
	static QString userIdAsString();

	//Returns user role from NGSD
	static QString role();
	//Returns if a user is logged in (and that the NGSD is enabled)
	static bool active();

	//User is logged in
	static void login(QString user, bool test_db = false);
	//Log out user
	static void logout();

	//Checks if the user role is in the given role list. If not, an exception is thrown.
	static void checkRoleIn(QStringList roles);

private:
	LoginManager();
	static LoginManager& instance();

	QString user_;
	int user_id_;
	QString role_;
};

#endif // LOGINMANAGER_H
