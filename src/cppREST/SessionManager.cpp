#include "SessionManager.h"
#include "Helper.h"

SessionManager::SessionManager()
	: backup_file_(Helper::openFileForWriting(ServerHelper::getSessionBackupFileName(), false, true))
	, session_store_()
	, current_client_info_()
	, current_notification_()
{	
}

SessionManager& SessionManager::instance()
{
	static SessionManager session_manager;	
	return session_manager;
}

void SessionManager::rewriteFile()
{
	instance().mutex_.lock();
    instance().backup_file_.data()->close();

    if (instance().backup_file_.data()->remove())
    {
        Log::info("Session backup file has been removed");
        instance().backup_file_ = Helper::openFileForWriting(ServerHelper::getSessionBackupFileName(), false, true);

        QMapIterator<QString, Session> i(instance().session_store_);
        QTextStream out(instance().backup_file_.data());

        while (i.hasNext())
        {
            i.next();
            if (i.value().user_id > 0)
            {
                out << i.key() << "\t" << i.value().user_id << "\t" << i.value().user_login << "\t" << i.value().user_name << "\t" << i.value().login_time.toSecsSinceEpoch() << "\t" << i.value().is_for_db_only << "\n";
            }
        }

    }
    else
    {
        Log::error("Could not remove the session backup file");
    }

	instance().mutex_.unlock();
}

void SessionManager::saveSessionToFile(QString id, Session in)
{
	QTextStream out(instance().backup_file_.data());
	if (!in.isEmpty())
    {
        out << id << "\t" << in.user_id << "\t" << in.user_login << "\t" << in.user_name << "\t" << in.login_time.toSecsSinceEpoch() << "\t" << in.is_for_db_only << "\n";
	}
}

void SessionManager::restoreFromFile()
{
	if (QFile(ServerHelper::getSessionBackupFileName()).exists())
	{		
		int restored_items = 0;
		if (instance().backup_file_.data()->isOpen()) instance().backup_file_.data()->close();
		instance().backup_file_ = Helper::openFileForReading(ServerHelper::getSessionBackupFileName());
		while(!instance().backup_file_.data()->atEnd())
		{
			QString line = instance().backup_file_.data()->readLine();
			if(line.isEmpty()) break;

			QList<QString> line_list = line.split("\t");
            if (line_list.count() > 5)
			{
				bool ok;
				restored_items++;
                addNewSession(line_list[0], Session(line_list[1].toInt(), line_list[2], line_list[3], QDateTime::fromSecsSinceEpoch(line_list[4].toLongLong(&ok,10)), line_list[5].toInt()), false);
			}
		}
		Log::info("Number of restored sessions: " + QString::number(restored_items));
		instance().backup_file_.data()->close();

        removeExpiredSessions();
	}
	else
	{
		Log::info("Session backup has not been found: nothing to restore");
	}
}

void SessionManager::addNewSession(QString id, Session in, bool save_to_file)
{
	instance().mutex_.lock();
	instance().session_store_.insert(id, in);
	if (save_to_file) saveSessionToFile(id, in);
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

void SessionManager::removeExpiredSessions()
{
	QList<QString> to_be_removed {};
	QMapIterator<QString, Session> i(instance().session_store_);
	while (i.hasNext()) {
		i.next();
		if (isSessionExpired(i.value()))
		{
			to_be_removed.append(i.key());
		}
	}

    Log::info("Number of removed sessions: " + QString::number(to_be_removed.length()));

	for (int i = 0; i < to_be_removed.count(); ++i)
	{
		removeSession(to_be_removed[i]);
	}

    //wipe the backup file out and write active sessions
    rewriteFile();
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
