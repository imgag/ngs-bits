#include "SessionManager.h"

SessionManager::SessionManager()
	: session_store_()
{
}

SessionManager& SessionManager::instance()
{
	static SessionManager session_manager;
	return session_manager;
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
