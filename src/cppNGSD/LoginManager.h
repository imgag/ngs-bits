#ifndef LOGINMANAGER_H
#define LOGINMANAGER_H

#include <QString>
#include "cppNGSD_global.h"
#include "HttpRequestHandler.h"

///NGSD login manager (singleton)
class CPPNGSDSHARED_EXPORT LoginManager
{
public:
	//Returns user login from NGSD
	static QString userLogin();
	//Returns the full user name from NGSD
	static QString userName();
	//Returns user ID from NGSD
	static int userId();
	//Returns user ID from NGSD (as string)
	static QString userIdAsString();

	//Returns a secure token generated for the given user
	static QString userToken();
	static QString userPassword();

	//Returns if a user is logged in (and that the NGSD is enabled)
	static bool active();

	//User is logged in
	static void login(QString user, QString password, bool test_db = false);
	//Updates the user's session before it expires
	static void renewLogin();
	//Secure token to access the database
	static QString dbToken();
	//Log out user
	static void logout();

	static QString ngsdHostName();
	static int ngsdPort();
	static QString ngsdName();
	static QString ngsdUser();
	static QString ngsdPassword();

	static bool genlab_mssql();
	static QString genlabHost();
	static int genlabPort();
	static QString genlabName();
	static QString genlabUser();
	static QString genlabPassword();

	//Checks if the logged-in user has one of the given roles. If not, an exception is thrown.
	static void checkRoleIn(QStringList roles);
	//Checks if the logged-in user has none of the given roles. If has has, an exception is thrown.
	static void checkRoleNotIn(QStringList roles);

private:
	LoginManager();
	static LoginManager& instance();
	static QByteArray sendPostApiRequest(QString path, QString content, HttpHeaders add_headers);
	static QByteArray sendGetApiRequest(QString path, HttpHeaders add_headers);
    static void setAllTokens(const QString& user, const QString& password);

	//User info
	QString user_login_;
	QString user_name_;
	int user_id_;
	QString user_token_;
	QString user_password_;

	//Token for requesting database credentials
	QString db_token_;

	//NGSD info
	QString ngsd_host_name_;
	int ngsd_port_;
	QString ngsd_name_;
	QString ngsd_user_;
	QString ngsd_password_;

	//Genlab info
	bool genlab_mssql_;
	QString genlab_host_;
	int genlab_port_;
	QString genlab_name_;
	QString genlab_user_;
	QString genlab_password_;
};

#endif // LOGINMANAGER_H
