#include "SessionManager.h"
#include "Helper.h"

SessionManager::SessionManager()
    : session_store_()
	, current_client_info_()
	, current_notification_()
{	
}

SessionManager& SessionManager::instance()
{
	static SessionManager session_manager;	
	return session_manager;
}

void SessionManager::restoreFromFile()
{
	if (QFile(ServerHelper::getSessionBackupFileName()).exists())
    {
        qint64 restored_items = 0;

        QSharedPointer<QFile> backup_file = Helper::openFileForReading(ServerHelper::getSessionBackupFileName());
        while(!backup_file.data()->atEnd())
		{
            QString line = backup_file.data()->readLine();
			if(line.isEmpty()) break;

			QList<QString> line_list = line.split("\t");
            if (line_list.count() > 5)
			{
				bool ok;
                Session current_session = Session(line_list[1].toInt(), line_list[2], line_list[3], QDateTime::fromSecsSinceEpoch(line_list[4].toLongLong(&ok,10)), line_list[5].toInt());
                if (!isSessionExpired(current_session))
                {
                    addNewSession(line_list[0], current_session);
                    restored_items++;
                }
			}
		}
		Log::info("Number of restored sessions: " + QString::number(restored_items));
        backup_file.data()->close();
	}
	else
	{
		Log::info("Session backup has not been found: nothing to restore");
	}
}

void SessionManager::addNewSession(QString id, Session in)
{
	instance().mutex_.lock();
    instance().session_store_.insert(id, in);
	instance().mutex_.unlock();
}

void SessionManager::removeSession(QString id)
{
	if (instance().session_store_.contains(id))
	{
		instance().mutex_.lock();
		instance().session_store_.remove(id);
		instance().mutex_.unlock();
	}
}

Session SessionManager::getSessionBySecureToken(QString token)
{
	QMapIterator<QString, Session> i(instance().session_store_);
	while (i.hasNext())
	{
		i.next();
		if (i.key() == token)
		{
			return i.value();
		}
	}
	return Session();
}

bool SessionManager::isSessionExpired(Session in)
{
	// Session lifetime in seconds
	qint64 valid_period = ServerHelper::getNumSettingsValue("session_duration");
	if (valid_period == 0) valid_period = DEFAULT_VALID_PERIOD; // default value, if not set in the config
	if (in.login_time.addSecs(valid_period).toSecsSinceEpoch() < QDateTime::currentDateTime().toSecsSinceEpoch())
	{
		return true;
	}
	return false;
}

bool SessionManager::isSessionExpired(QString token)
{
	return isSessionExpired(getSessionBySecureToken(token));
}

bool SessionManager::isTokenReal(QString token)
{
	QMapIterator<QString, Session> i(instance().session_store_);
	while (i.hasNext())
	{
		i.next();
		if (i.key() == token)
		{
			return true;
		}
	}
	return false;
}

QMap<QString, Session> SessionManager::removeExpiredSessions()
{
    QMap<QString, Session> tmp_storage = {};
    QList<QString> to_be_removed {};

    Log::info("Starting to cleanup sessions");
    instance().mutex_.lock();
	QMapIterator<QString, Session> i(instance().session_store_);
    while (i.hasNext())
    {
		i.next();
		if (isSessionExpired(i.value()))
		{
			to_be_removed.append(i.key());
		}
	}
	for (int i = 0; i < to_be_removed.count(); ++i)
    {
        instance().session_store_.remove(to_be_removed[i]);
	}
    tmp_storage = instance().session_store_;
    instance().mutex_.unlock();

    Log::info("Number of removed sessions: " + QString::number(to_be_removed.length()));
    return tmp_storage;
}

ClientInfo SessionManager::getCurrentClientInfo()
{
	return instance().current_client_info_;
}

void SessionManager::setCurrentClientInfo(ClientInfo info)
{
	if (!info.isEmpty())
	{
		instance().mutex_.lock();
		instance().current_client_info_ = info;
		instance().mutex_.unlock();
	}
}

UserNotification SessionManager::getCurrentNotification()
{
	return instance().current_notification_;
}

void SessionManager::setCurrentNotification(QString message)
{
	instance().mutex_.lock();
	instance().current_notification_ = UserNotification(ServerHelper::generateUniqueStr(), message);
	instance().mutex_.unlock();
}
