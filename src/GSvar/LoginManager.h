#ifndef LOGINMANAGER_H
#define LOGINMANAGER_H

#include <QString>

///NGSD login manager (singleton)
class LoginManager
{
public:
	//Returns user identifier from NGSD (not name)
	static QString user();
	//Returns user role from NGSD
	static QString role();
	//Returns if a user is logged in (and that the NGSD is enabled)
	static bool active();

	//User is logged in
	static void login(QString user);
	//Log out user
	static void logout();

	//Checks if the user role is in the given role list. If not, an exception is thrown.
	static void checkRoleIn(QStringList roles);

private:
	LoginManager();
	static LoginManager& instance();

	QString user_;
	QString role_;
};

#endif // LOGINMANAGER_H
