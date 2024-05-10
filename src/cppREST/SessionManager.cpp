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
    FileDbManager().addSession(in);
}

void SessionManager::removeSession(QString id)
{
    FileDbManager().removeSession(id);
}

Session SessionManager::getSessionBySecureToken(QString token)
{
    return FileDbManager().getSession(token);
}

bool SessionManager::isValidSession(QString token)
{
    Session cur_session = FileDbManager().getSession(token);
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

int SessionManager::removeExpiredSessions()
{
    qint64 valid_period = ServerHelper::getNumSettingsValue("session_duration");
    if (valid_period == 0) valid_period = DEFAULT_VALID_PERIOD; // default value, if not set in the config

    int removed_count = 0;
    Log::info("Starting to cleanup sessions");
    FileDbManager file_db_manager = FileDbManager();
    QList<Session> all_sessions = file_db_manager.getAllSessions();
    for (int i = 0; i < all_sessions.size(); i++)
    {
        if (all_sessions[i].login_time.addSecs(valid_period).toSecsSinceEpoch() < QDateTime::currentDateTime().toSecsSinceEpoch())
        {
            continue;
        }

        file_db_manager.removeSession(all_sessions[i].string_id);
        removed_count++;
    }

    Log::info("Number of removed sessions: " + QString::number(removed_count));
    return removed_count;
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
