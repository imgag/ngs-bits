#include "IgvSessionManager.h"

IgvSessionManager::IgvSessionManager()
{
}

IgvSessionManager::~IgvSessionManager()
{
}

IgvSessionManager& IgvSessionManager::instance()
{
	static IgvSessionManager instance;
	return instance;
}

IGVSession* IgvSessionManager::create(QWidget* parent, const QString& name, const QString& app, const QString& host, const QString& genome)
{
	instance().mutex_.lock();

	//determine port
	int port = Settings::integer("igv_port");
	port += 1000 * (instance().session_list_.count()+1); //specific port for each IGV session
	if (LoginManager::active()) port += LoginManager::userId(); //specific port for each user (needed e.g. for citrix)

	//create session
	QSharedPointer<IGVSession> session = QSharedPointer<IGVSession>(new IGVSession(parent, name, app, host, port, genome));
	instance().session_list_.append(session);
	instance().mutex_.unlock();

	return session.data();
}

IGVSession& IgvSessionManager::get(const int& session_index)
{
	QSharedPointer<IGVSession> session;

	instance().mutex_.lock();
	if (session_index<0 || session_index>=instance().session_list_.count()) THROW(ProgrammingException, "Invalid session index has been provided!");
	session = instance().session_list_[session_index];
	instance().mutex_.unlock();

	return *(session.data());
}

int IgvSessionManager::count()
{
    instance().mutex_.lock();
    int session_count = instance().session_list_.count();
    instance().mutex_.unlock();

    return session_count;
}

void IgvSessionManager::clearAll()
{
    instance().mutex_.lock();
    foreach (QSharedPointer<IGVSession> session, instance().session_list_)
    {
		if (session.data()->isIgvRunning())
		{
			session.data()->clear();
		}
    }
    instance().mutex_.unlock();
}

