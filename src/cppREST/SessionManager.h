#ifndef SESSIONMANAGER_H
#define SESSIONMANAGER_H

#include "cppREST_global.h"
#include <QDateTime>
#include "ClientHelper.h"
#include "ServerHelper.h"
#include "Exceptions.h"
#include "ServerDB.h"
#include "ThreadSafeHashMap.h"

class CPPRESTSHARED_EXPORT SessionManager
{
public:
    static const qint64 DEFAULT_VALID_PERIOD = 3600; // in seconds
    static void addNewSession(Session in);
	static void removeSession(QString id);	
	static Session getSessionBySecureToken(QString token);
    static QList<Session> getAllSessions();

    static bool isValidSession(QString token);
    static void removeExpiredSessions();

	static ClientInfo getCurrentClientInfo();
	static void setCurrentClientInfo(ClientInfo info);

	static UserNotification getCurrentNotification();
	static void setCurrentNotification(QString message);

protected:
	SessionManager();

private:
    static SessionManager& instance();	
	ClientInfo current_client_info_;
	UserNotification current_notification_;
    ThreadSafeHashMap<QString, Session> session_storage_;
};

#endif // SESSIONMANAGER_H
