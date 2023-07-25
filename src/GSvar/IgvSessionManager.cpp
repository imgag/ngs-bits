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

int IgvSessionManager::createIGVSession(int port, bool is_initialized, QString genome)
{
	if (port<=0) THROW(ProgrammingException, "Port number is not set!");
	if (genome.isEmpty()) THROW(ProgrammingException, "Genome path is not set!");

	instance().mutex_.lock();
	instance().session_list_.append(IGVSession{port, is_initialized, genome});
	instance().mutex_.unlock();
	return instance().session_list_.count()-1;
}

void IgvSessionManager::removeIGVSession(int session_index)
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

int IgvSessionManager::getIGVPort(int session_index)
{
	if (session_index<0) THROW(ProgrammingException, "Invalid session index has not been provided!");
	if (instance().session_list_.count()>=(session_index+1))
	{
		return instance().session_list_[session_index].port;
	}
	return -1;
}

void IgvSessionManager::setIGVPort(int port, int session_index)
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

QString IgvSessionManager::getIGVGenome(int session_index)
{
    if (session_index<0) THROW(ProgrammingException, "Invalid session index has been provided!");
	if (instance().session_list_.count()>=(session_index+1))
	{
		return instance().session_list_[session_index].genome;
	}
	return "";
}

void IgvSessionManager::setIGVGenome(QString genome, int session_index)
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
		instance().mutex_.unlock();
	}
}
