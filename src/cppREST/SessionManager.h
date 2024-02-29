#ifndef SESSIONMANAGER_H
#define SESSIONMANAGER_H

#include "cppREST_global.h"
#include <QMap>
#include <QDateTime>
#include <QMutex>
#include "ClientHelper.h"
#include "ServerHelper.h"
#include "Exceptions.h"

struct CPPRESTSHARED_EXPORT Session
{
	int user_id;
    QString user_login;
    QString user_name;
	QDateTime login_time;
    bool is_for_db_only;

	Session()
		: user_id()
        , user_login()
        , user_name()
		, login_time()
		, is_for_db_only()
	{
	}

    Session(const int& user_id_, const QString& user_login, const QString& user_name, const QDateTime login_time_, const bool is_for_db_only_ = false)
		: user_id(user_id_)
        , user_login(user_login)
        , user_name(user_name)
		, login_time(login_time_)
		, is_for_db_only(is_for_db_only_)
	{
	}

	bool isEmpty()
	{
		return user_id==0 && login_time.isNull() && !is_for_db_only;
	}
};

class CPPRESTSHARED_EXPORT SessionManager
{
public:
    static const qint64 DEFAULT_VALID_PERIOD = 3600; // in seconds
	static void restoreFromFile();
    static void addNewSession(QString id, Session in);
	static void removeSession(QString id);	
	static Session getSessionBySecureToken(QString token);
    static bool isSessionExpired(Session in);

	static bool isTokenReal(QString token);
    static QMap<QString, Session> removeExpiredSessions();

	static ClientInfo getCurrentClientInfo();
	static void setCurrentClientInfo(ClientInfo info);

	static UserNotification getCurrentNotification();
	static void setCurrentNotification(QString message);

protected:
	SessionManager();

private:
    static SessionManager& instance();
	QMutex mutex_;
	QMap<QString, Session> session_store_;
	ClientInfo current_client_info_;
	UserNotification current_notification_;
};

#endif // SESSIONMANAGER_H
