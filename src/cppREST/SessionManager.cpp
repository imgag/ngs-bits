#include "SessionManager.h"
#include "Helper.h"

SessionManager::SessionManager()
    : current_client_info_()
	, current_notification_()
{
}

SessionManager& SessionManager::instance()
{
    static SessionManager session_manager;
	return session_manager;
}

void SessionManager::addNewSession(Session in)
{
    ServerDbManager::addSession(in);
}

void SessionManager::removeSession(QString id)
{
    ServerDbManager::removeSession(id);
}

Session SessionManager::getSessionBySecureToken(QString token)
{
    return ServerDbManager::getSession(token);
}

bool SessionManager::isValidSession(QString token)
{
    Session cur_session = ServerDbManager::getSession(token);
    if (cur_session.isEmpty())
    {
        return false;
    }

    qint64 valid_period = ServerHelper::getNumSettingsValue("session_duration");
    if (valid_period == 0) valid_period = DEFAULT_VALID_PERIOD; // default value, if not set in the config
    if (cur_session.login_time.addSecs(valid_period).toSecsSinceEpoch() < QDateTime::currentDateTime().toSecsSinceEpoch())
    {
        return false;
    }

    return true;
}

void SessionManager::removeExpiredSessions()
{
    qint64 valid_period = ServerHelper::getNumSettingsValue("session_duration");
    if (valid_period == 0) valid_period = DEFAULT_VALID_PERIOD; // default value, if not set in the config

    Log::info("Starting to cleanup sessions");
    int current_count = ServerDbManager::getSessionsCount();
    Log::info("Number of active sessions: " + QString::number(current_count));
    ServerDbManager::removeSessionsOlderThan(QDateTime::currentDateTime().toSecsSinceEpoch()-valid_period);

    int new_count = ServerDbManager::getSessionsCount();
    Log::info("Number of active sessions after the cleanup: " + QString::number(new_count));
}

ClientInfo SessionManager::getCurrentClientInfo()
{
	return instance().current_client_info_;
}

void SessionManager::setCurrentClientInfo(ClientInfo info)
{
	if (!info.isEmpty())
	{		
		instance().current_client_info_ = info;		
	}
}

UserNotification SessionManager::getCurrentNotification()
{
	return instance().current_notification_;
}

void SessionManager::setCurrentNotification(QString message)
{	
	instance().current_notification_ = UserNotification(ServerHelper::generateUniqueStr(), message);
}
