#include "SessionManager.h"
#include "Helper.h"

SessionManager::SessionManager()
	: output_file_(Helper::openFileForWriting(ServerHelper::getSessionBackupFileName(), false, true))
	, session_store_()
{	
}

SessionManager& SessionManager::instance()
{
	static SessionManager session_manager;	
	return session_manager;
}

void SessionManager::saveEverythingToFile()
{
	instance().mutex_.lock();
	QMapIterator<QString, Session> i(instance().session_store_);
	QTextStream out(instance().output_file_.data());

	while (i.hasNext())
	{
		i.next();
		if (i.value().user_id > 0)
		{
			out << i.key() << "\t" << i.value().user_id << "\t" << i.value().login_time.toString() << "\t" << i.value().is_for_db_only << "\n";
		}
	}
	instance().mutex_.unlock();
}

void SessionManager::saveSessionToFile(QString id, Session in)
{
	QTextStream out(instance().output_file_.data());
	if (!in.isEmpty())
	{
		out << id << "\t" << in.user_id << "\t" << in.login_time.toString() << "\t" << in.is_for_db_only << "\n";
	}
}

void SessionManager::restoreFromFile(bool remove_backup)
{
	if (QFile(ServerHelper::getSessionBackupFileName()).exists())
	{
		QMapIterator<QString, Session> i(instance().session_store_);
		QSharedPointer<QFile> input_file = Helper::openFileForReading(ServerHelper::getSessionBackupFileName());
		while(!input_file->atEnd())
		{
			QString line = input_file->readLine();
			if(line.isEmpty()) break;

			QList<QString> line_list = line.split("\t");
			if (line_list.count() > 3)
			{
				addNewSession(line_list[0], Session(line_list[1].toInt(), QDateTime::fromString(line_list[2]), line_list[3].toInt()), false);
			}
		}

		if (!remove_backup) return;
		if (!QFile(ServerHelper::getSessionBackupFileName()).remove())
		{
			Log::error("Could not remove session backup file: " + ServerHelper::getSessionBackupFileName());
		}
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
	else
	{
		THROW(ArgumentException, "Secure token could not be found");
	}
}

void SessionManager::removeSession(int user_id, QDateTime login_time)
{
	QMapIterator<QString, Session> i(instance().session_store_);
	QString session_id;
	while (i.hasNext())
	{
		i.next();
		if ((i.value().user_id == user_id) && (i.value().login_time == login_time))
		{
			session_id = i.key();
			break;
		}
	}
	if (!session_id.isEmpty())
	{
		removeSession(session_id);
	}
}

Session SessionManager::getSessionByUserId(QString id)
{
	QMapIterator<QString, Session> i(instance().session_store_);
	while (i.hasNext())
	{
		i.next();
		if (i.value().user_id == id)
		{
			return i.value();
		}
	}
	return Session();
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

bool SessionManager::isSessionExpired(QString token)
{
	Session in = getSessionBySecureToken(token);
	qint64 valid_period = ServerHelper::getNumSettingsValue("session_duration");
	if (valid_period == 0) valid_period = 3600;

	if (in.login_time.addSecs(valid_period).toSecsSinceEpoch() < QDateTime::currentDateTime().toSecsSinceEpoch())
	{
		qDebug() << "Secure token has expired. Session is being removed";
		removeSession(in.user_id, in.login_time);
		return true;
	}
	return false;
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
