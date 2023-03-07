#ifndef SESSIONMANAGER_H
#define SESSIONMANAGER_H

#include "cppREST_global.h"
#include <QMap>
#include <QDateTime>
#include <QMutex>
#include "NGSHelper.h"
#include "ServerHelper.h"
#include "Exceptions.h"

struct CPPRESTSHARED_EXPORT Session
{
	int user_id;
	QDateTime login_time;
	bool is_for_db_only;

	Session()
		: user_id()
		, login_time()
		, is_for_db_only()
	{
	}

	Session(const int& user_id_, const QDateTime login_time_, const bool is_for_db_only_ = false)
		: user_id(user_id_)
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
	static void saveEverythingToFile();
	static void saveSessionToFile(QString id, Session in);
	static void restoreFromFile();
	static void addNewSession(QString id, Session in, bool save_to_file = true);
	static void removeSession(QString id);	
	static Session getSessionBySecureToken(QString token);
	static bool isSessionExpired(Session in);
	static bool isSessionExpired(QString token);

	static bool isTokenReal(QString token);
	static void removeExpiredSessions();

	static ClientInfo getCurrentClientInfo();
	static void setCurrentClientInfo(ClientInfo info);

	static UserNotification getCurrentNotification();
	static void setCurrentNotification(QString message);

protected:
	SessionManager();

private:
	static SessionManager& instance();
	QSharedPointer<QFile> backup_file_;
	QMutex mutex_;
	QMap<QString, Session> session_store_;
	ClientInfo current_client_info_;
	UserNotification current_notification_;
};

#endif // SESSIONMANAGER_H
