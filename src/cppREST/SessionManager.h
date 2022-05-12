#ifndef SESSIONMANAGER_H
#define SESSIONMANAGER_H

#include "cppREST_global.h"
#include <QMap>
#include <QDebug>
#include <QDateTime>
#include <QMutex>
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
		return ((this->user_id == 0) && (this->login_time.isNull()) && (!this->is_for_db_only));
	}
};

class CPPRESTSHARED_EXPORT SessionManager
{
public:
	static void addNewSession(QString id, Session in);
	static void removeSession(QString id);
	static void removeSession(int user_id, QDateTime login_time);
	static Session getSessionByUserId(QString id);
	static Session getSessionBySecureToken(QString token);
	static bool isSessionExpired(QString token);
	static bool isTokenReal(QString token);

protected:
	SessionManager();

private:
	QMutex mutex_;

	static SessionManager& instance();
	QMap<QString, Session> session_store_;
};

#endif // SESSIONMANAGER_H
