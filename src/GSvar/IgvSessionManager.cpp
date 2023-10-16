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

void IgvSessionManager::create(QWidget *parent, Ui::MainWindow parent_ui, const QString& name, const QString& app, const QString& host, const QString& genome)
{	
	if (genome.isEmpty()) THROW(ProgrammingException, "Genome path is not set!");

	instance().mutex_.lock();
    instance().session_list_.append(QSharedPointer<IGVSession>(new IGVSession(parent, parent_ui, name, app, host, findAvailablePortForIGV(), genome)));
	instance().mutex_.unlock();
}

void IgvSessionManager::remove(const int& session_index)
{
	if ((session_index<0) || (session_index>instance().session_list_.count()-1)) THROW(ProgrammingException, "Invalid session index!");
	instance().mutex_.lock();
    instance().session_list_.removeAt(session_index);
    instance().mutex_.unlock();
}

int IgvSessionManager::indexByName(const QString& name)
{
    int index = -1;
    instance().mutex_.lock();
    for (int i = 0; i < instance().session_list_.count(); i++)
    {
        if (instance().session_list_[i].data()->getName().toLower() == name.toLower()) index = i;
    }
    instance().mutex_.unlock();
    return index;
}

int IgvSessionManager::findAvailablePortForIGV()
{
	int port = Settings::integer("igv_port");
	if (LoginManager::active())
	{
		port += LoginManager::userId();
	}
    port += instance().session_list_.count() * 1000 + 1000;
    return port;
}

IGVSession& IgvSessionManager::get(const int& session_index)
{
    if (session_index<0 || instance().session_list_.count()<(session_index-1)) THROW(ProgrammingException, "Invalid session index has been provided!");
    return *(instance().session_list_[session_index].data());
}

int IgvSessionManager::count()
{
    instance().mutex_.lock();
    int session_count = instance().session_list_.count();
    instance().mutex_.unlock();

    return session_count;
}

void IgvSessionManager::resetIGVInitialized()
{
    instance().mutex_.lock();
    foreach (QSharedPointer<IGVSession> session, instance().session_list_)
    {
        session.data()->clearHistory();
        session.data()->setIGVInitialized(false);
    }
    instance().mutex_.unlock();
}

bool IgvSessionManager::hasAtLeastOneActiveIGV()
{
    bool hasActiveInstance = false;
    instance().mutex_.lock();
    foreach (QSharedPointer<IGVSession> session, instance().session_list_)
    {
        if (session.data()->hasRunningCommands())
        {
            hasActiveInstance = true;
            break;
        }
    }
    instance().mutex_.unlock();

    return hasActiveInstance;
}

