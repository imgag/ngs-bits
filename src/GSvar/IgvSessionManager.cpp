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

int IgvSessionManager::create(QString app, QString host, int port, QString genome, bool is_initialized)
{
	if (port<=0) THROW(ProgrammingException, "Port number is not set!");
	if (genome.isEmpty()) THROW(ProgrammingException, "Genome path is not set!");

	instance().mutex_.lock();
    instance().session_list_.append(IGVSession{app, host, port, genome, QSharedPointer<IGVCommandExecutor>(new IGVCommandExecutor(app, host, port)), is_initialized});
	instance().mutex_.unlock();
	return instance().session_list_.count()-1;
}

void IgvSessionManager::remove(int session_index)
{
	if ((session_index<0) || (session_index>instance().session_list_.count()-1)) THROW(ProgrammingException, "Invalid session index!");
	instance().mutex_.lock();
	instance().session_list_.removeAt(session_index);
	instance().mutex_.unlock();
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

QString IgvSessionManager::getHost(int session_index)
{
    if (session_index<0) THROW(ProgrammingException, "Invalid session index has been provided!");
    if (instance().session_list_.count()>=(session_index+1))
    {
        return instance().session_list_[session_index].host;
    }
    return "";
}

void IgvSessionManager::setHost(QString host, int session_index)
{
    if (host.isEmpty()) THROW(ProgrammingException, "Host is not set!");
    if (session_index<0) THROW(ProgrammingException, "Invalid session index has not been provided!");
    if (instance().session_list_.count()>=(session_index-1))
    {
        instance().mutex_.lock();
        instance().session_list_[session_index].host = host;
        instance().mutex_.unlock();
    }
}

int IgvSessionManager::getPort(int session_index)
{
	if (session_index<0) THROW(ProgrammingException, "Invalid session index has not been provided!");
	if (instance().session_list_.count()>=(session_index+1))
	{
		return instance().session_list_[session_index].port;
	}
	return -1;
}

void IgvSessionManager::setPort(int port, int session_index)
{
	if (port<=0) THROW(ProgrammingException, "Port number is not set!");
	if (session_index<0) THROW(ProgrammingException, "Invalid session index has not been provided!");
	if (instance().session_list_.count()>=(session_index-1))
	{
		instance().mutex_.lock();
		instance().session_list_[session_index].port = port;
		instance().mutex_.unlock();
	}
}

QString IgvSessionManager::getGenome(int session_index)
{
    if (session_index<0) THROW(ProgrammingException, "Invalid session index has been provided!");
	if (instance().session_list_.count()>=(session_index+1))
	{
		return instance().session_list_[session_index].genome;
	}
	return "";
}

void IgvSessionManager::setGenome(QString genome, int session_index)
{
	if (genome.isEmpty()) THROW(ProgrammingException, "Genome is not set!");
    if (session_index<0) THROW(ProgrammingException, "Invalid session index has been provided!");
	if (instance().session_list_.count()>=(session_index-1))
	{
		instance().mutex_.lock();
		instance().session_list_[session_index].genome = genome;
        instance().mutex_.unlock();
    }
}

QSharedPointer<IGVCommandExecutor> IgvSessionManager::getCommandExecutor(int session_index)
{
    if (session_index<0 || instance().session_list_.count()<(session_index-1)) THROW(ProgrammingException, "Invalid session index has been provided!");
    return instance().session_list_[session_index].command_executor;
}

bool IgvSessionManager::isIGVInitialized(int session_index)
{
    if (session_index<0) THROW(ProgrammingException, "Invalid session index has been provided!");

	if (instance().session_list_.count()>=(session_index+1))
	{
		return instance().session_list_[session_index].is_initialized;
	}

	return false;
}

void IgvSessionManager::setIGVInitialized(bool is_initialized, int session_index)
{
    if (session_index<0) THROW(ProgrammingException, "Invalid session index has been provided!");

	if (instance().session_list_.count()>=(session_index+1))
	{
		instance().mutex_.lock();
		instance().session_list_[session_index].is_initialized = is_initialized;
        if (!is_initialized) instance().session_list_[session_index].command_executor.data()->clearHistory();
		instance().mutex_.unlock();
	}
}
