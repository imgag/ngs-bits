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

Session SessionManager::getSessionByUserId(QString id)
{
	QMapIterator<QString, Session> i(instance().session_store_);
	while (i.hasNext()) {
		i.next();
		if (i.value().user_id == id)
		{
			return i.value();
		}
	}
	qDebug() << "Session return user id";
	return Session();
}

Session SessionManager::getSessionBySecureToken(QString token)
{
	QMapIterator<QString, Session> i(instance().session_store_);
	while (i.hasNext()) {
		i.next();
		if (i.key() == token)
		{
			return i.value();
		}
	}

	qDebug() << "Session return by token";
	return Session();
}

bool SessionManager::isSessionExpired(Session in)
{
	qint64 valid_period = ServerHelper::getNumSettingsValue("session_duration");
	if (valid_period == 0) valid_period = 60;

	qDebug() << "Login time: " << in.login_time;
	qDebug() << "User id: " << in.user_id;
	qDebug() << "Current time: " << QDateTime::currentDateTime().toSecsSinceEpoch();
	qDebug() << "Valid period: " << valid_period;

	if (in.login_time.addSecs(valid_period) > QDateTime::currentDateTime())
	{
		return true;
	}
	qDebug() << "Secure token has expired";
	return false;
}

bool SessionManager::isTokenValid(QString token)
{
	return isSessionExpired(getSessionBySecureToken(token));
}

bool SessionManager::hasValidToken(QString user_id)
{
	return isSessionExpired(getSessionByUserId(user_id));
}
