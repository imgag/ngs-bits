#include "SessionManager.h"
#include "Helper.h"
#include "Settings.h"
#include "Log.h"
#include "ServerHelper.h"

SessionManager::SessionManager()
    : current_client_info_()
	, current_notification_()
    , session_storage_()
{
}

SessionManager& SessionManager::instance()
{
    static SessionManager session_manager;
	return session_manager;
}

void SessionManager::addNewSession(Session in)
{
    instance().session_storage_.insert(in.string_id, in);
}

void SessionManager::removeSession(QString id)
{
    if (instance().session_storage_.contains(id))
    {
        instance().session_storage_.remove(id);
    }
}

Session SessionManager::getSessionBySecureToken(QString token)
{
    if (instance().session_storage_.contains(token))
    {
        return instance().session_storage_.value(token);
    }
    return Session{};
}

QList<Session> SessionManager::getAllSessions()
{
    QList<Session> all_sessions;
    QList<QString> keys = instance().session_storage_.keys();
    for (int i = 0; i < keys.count(); i++)
    {
        all_sessions.append(instance().session_storage_.value(keys[i]));
    }
    return all_sessions;
}

bool SessionManager::isValidSession(QString token)
{
    Session cur_session = instance().getSessionBySecureToken(token);
    if (cur_session.isEmpty())
    {
        return false;
    }

    qint64 valid_period = 0;
    try
    {
        valid_period = Settings::integer("session_duration");
    }
    catch(ProgrammingException& e)
    {
        valid_period = DEFAULT_VALID_PERIOD;
        Log::warn(e.message() + " Using the default value: " + QString::number(valid_period));
    }

    if (cur_session.login_time.addSecs(valid_period).toSecsSinceEpoch() < QDateTime::currentDateTime().toSecsSinceEpoch())
    {
        return false;
    }

    return true;
}

void SessionManager::removeExpiredSessions()
{
    qint64 valid_period = 0;
    try
    {
        valid_period = Settings::integer("session_duration");
    }
    catch(ProgrammingException& e)
    {
        valid_period = DEFAULT_VALID_PERIOD;
        Log::warn(e.message() + " Using the default value: " + QString::number(valid_period));
    }

    Log::info("Starting to cleanup session");
    QList<QString> to_be_removed {};
    QList<Session> to_be_backedup {};

    QList<QString> keys = instance().session_storage_.keys();
    for (int i = 0; i < keys.count(); i++)
    {
        if (instance().session_storage_.value(keys[i]).login_time.toSecsSinceEpoch() < (QDateTime::currentDateTime().toSecsSinceEpoch()-valid_period))
        {
            to_be_removed.append(keys[i]);
        }
        else
        {
            to_be_backedup.append(instance().session_storage_.value(keys[i]));
        }
    }
    for (int i = 0; i < to_be_removed.count(); ++i)
    {
        instance().session_storage_.remove(to_be_removed[i]);
    }

    Log::info("Number of removed sessions: " + QString::number(to_be_removed.length()));
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
