#include "GlobalServiceProvider.h"

#include "Settings.h"
#include "DatabaseServiceLocal.h"
#include "DatabaseServiceRemote.h"
#include "StatisticsServiceLocal.h"
#include "StatisticsServiceRemote.h"
#include "MainWindow.h"
#include "GSvarHelper.h"

GlobalServiceProvider::GlobalServiceProvider()
  : file_location_provider_()
  , database_service_()
  , statistics_service_()
{
	if (NGSHelper::isClientServerMode())
	{		
		database_service_ = QSharedPointer<DatabaseService>(new DatabaseServiceRemote());
		statistics_service_ = QSharedPointer<StatisticsService>(new StatisticsServiceRemote());
	}
	else
	{
		database_service_ = QSharedPointer<DatabaseService>(new DatabaseServiceLocal());
		statistics_service_ = QSharedPointer<StatisticsService>(new StatisticsServiceLocal());
	}
}

GlobalServiceProvider::~GlobalServiceProvider()
{
}

GlobalServiceProvider& GlobalServiceProvider::instance()
{
	static GlobalServiceProvider instance;

	return instance;
}

void GlobalServiceProvider::setFileLocationProvider(QSharedPointer<FileLocationProvider> file_location_provider)
{
	instance().file_location_provider_ = file_location_provider;
}

const FileLocationProvider& GlobalServiceProvider::fileLocationProvider()
{	
	if (instance().file_location_provider_.isNull())
	{
		THROW(ProgrammingException, "File location provider requested but not set!");
	}

	return *(instance().file_location_provider_);
}

void GlobalServiceProvider::clearFileLocationProvider()
{
	instance().file_location_provider_ = QSharedPointer<FileLocationProvider>();
}

const DatabaseService& GlobalServiceProvider::database()
{
	if (instance().database_service_.isNull())
	{
		THROW(ProgrammingException, "Database service requested but not set!");
	}
	return *(instance().database_service_);
}

StatisticsService& GlobalServiceProvider::statistics()
{
	if (instance().statistics_service_.isNull())
	{
		THROW(ProgrammingException, "Statistics service requested but not set!");
	}
	return *(instance().statistics_service_);
}

void GlobalServiceProvider::openProcessedSampleTab(QString processed_sample_name)
{
	foreach(QWidget* widget, QApplication::topLevelWidgets())
	{
		MainWindow* mw = qobject_cast<MainWindow*>(widget);
		if (mw!=nullptr)
		{
			mw->openProcessedSampleTab(processed_sample_name);
		}
	}
}

void GlobalServiceProvider::openRunTab(QString run_name)
{
	foreach(QWidget* widget, QApplication::topLevelWidgets())
	{
		MainWindow* mw = qobject_cast<MainWindow*>(widget);
		if (mw!=nullptr)
		{
			mw->openRunTab(run_name);
		}
	}
}

void GlobalServiceProvider::openGeneTab(QString symbol)
{
	foreach(QWidget* widget, QApplication::topLevelWidgets())
	{
		MainWindow* mw = qobject_cast<MainWindow*>(widget);
		if (mw!=nullptr)
		{
			mw->openGeneTab(symbol);
		}
	}
}

void GlobalServiceProvider::openVariantTab(Variant variant)
{
	foreach(QWidget* widget, QApplication::topLevelWidgets())
	{
		MainWindow* mw = qobject_cast<MainWindow*>(widget);
		if (mw!=nullptr)
		{
			mw->openVariantTab(variant);
		}
	}
}

void GlobalServiceProvider::openProjectTab(QString project_name)
{
	foreach(QWidget* widget, QApplication::topLevelWidgets())
	{
		MainWindow* mw = qobject_cast<MainWindow*>(widget);
		if (mw!=nullptr)
		{
			mw->openProjectTab(project_name);
		}
	}
}

void GlobalServiceProvider::openProcessingSystemTab(QString system_short_name)
{
	foreach(QWidget* widget, QApplication::topLevelWidgets())
	{
		MainWindow* mw = qobject_cast<MainWindow*>(widget);
		if (mw!=nullptr)
		{
			mw->openProcessingSystemTab(system_short_name);
		}
	}
}

void GlobalServiceProvider::executeCommandListInIGV(QStringList commands, bool init_if_not_done, int session_index)
{
	foreach(QWidget* widget, QApplication::topLevelWidgets())
	{
		MainWindow* mw = qobject_cast<MainWindow*>(widget);
		if (mw!=nullptr)
		{
			mw->executeIGVCommands(commands, init_if_not_done, session_index);
		}
	}
}

void GlobalServiceProvider::executeCommandInIGV(QString command, bool init_if_not_done, int session_index)
{
	executeCommandListInIGV(QStringList() << command, init_if_not_done, session_index);
}

void GlobalServiceProvider::gotoInIGV(QString region, bool init_if_not_done, int session_index)
{
	executeCommandInIGV("goto " + region, init_if_not_done, session_index);
}

void GlobalServiceProvider::loadFileInIGV(QString filename, bool init_if_not_done, bool is_virus_genome)
{
	//normalize local files
	filename = Helper::canonicalPath(filename);

	if (NGSHelper::isClientServerMode()) executeCommandInIGV("SetAccessToken " + LoginManager::userToken() + " *" + Settings::string("server_host") + "*", init_if_not_done, is_virus_genome);
	executeCommandInIGV("load \"" + NGSHelper::stripSecureToken(filename) + "\"", init_if_not_done, is_virus_genome);
}

int GlobalServiceProvider::createIGVSession(int port, bool is_initialized, QString genome)
{
	if (port<=0) THROW(ProgrammingException, "Port number is not set!");
	if (genome.isEmpty()) THROW(ProgrammingException, "Genome path is not set!");

	instance().mutex_.lock();
	instance().session_list_.append(IGVSession{port, is_initialized, genome});
	instance().mutex_.unlock();
	return instance().session_list_.count()-1;
}

void GlobalServiceProvider::removeIGVSession(int session_index)
{
	if ((session_index<0) || (session_index>instance().session_list_.count()-1)) THROW(ProgrammingException, "Invalid session index!");
	instance().mutex_.lock();
	instance().session_list_.removeAt(session_index);
	instance().mutex_.unlock();
}

int GlobalServiceProvider::findAvailablePortForIGV()
{
	int port = Settings::integer("igv_port");
	if (LoginManager::active())
	{
		port += LoginManager::userId();
	}
	port += instance().session_list_.count() * 1000 + 1000;
	return port;
}

int GlobalServiceProvider::getIGVPort(int session_index)
{
	if (session_index<0) THROW(ProgrammingException, "Invalid session index has not been provided!");
	if (instance().session_list_.count()>=(session_index+1))
	{
		return instance().session_list_[session_index].port;
	}
	return -1;
}

void GlobalServiceProvider::setIGVPort(int port, int session_index)
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

QString GlobalServiceProvider::getIGVGenome(int session_index)
{
	if (session_index<0) THROW(ProgrammingException, "Invalid session index has not been provided!");
	if (instance().session_list_.count()>=(session_index+1))
	{
		return instance().session_list_[session_index].genome;
	}
	return "";
}

void GlobalServiceProvider::setIGVGenome(QString genome, int session_index)
{
	if (genome.isEmpty()) THROW(ProgrammingException, "Genome is not set!");
	if (session_index<0) THROW(ProgrammingException, "Invalid session index has not been provided!");
	if (instance().session_list_.count()>=(session_index-1))
	{
		instance().mutex_.lock();
		instance().session_list_[session_index].genome = genome;
		instance().mutex_.unlock();
	}
}

bool GlobalServiceProvider::isIGVInitialized(int session_index)
{
	if (session_index<0) THROW(ProgrammingException, "Invalid session index has not been provided!");

	if (instance().session_list_.count()>=(session_index+1))
	{
		return instance().session_list_[session_index].is_initialized;
	}

	return false;
}

void GlobalServiceProvider::setIGVInitialized(bool is_initialized, int session_index)
{
	if (session_index<0) THROW(ProgrammingException, "Invalid session index has not been provided!");

	if (instance().session_list_.count()>=(session_index+1))
	{
		instance().mutex_.lock();
		instance().session_list_[session_index].is_initialized = is_initialized;
		instance().mutex_.unlock();
	}
}

void GlobalServiceProvider::openGSvarViaNGSD(QString processed_sample_name, bool search_multi)
{
	foreach(QWidget* widget, QApplication::topLevelWidgets())
	{
		MainWindow* mw = qobject_cast<MainWindow*>(widget);
		if (mw!=nullptr)
		{
			mw->openProcessedSampleFromNGSD(processed_sample_name, search_multi);
		}
	}
}

void GlobalServiceProvider::addModelessDialog(QSharedPointer<QDialog> dlg, bool maximize)
{
	foreach(QWidget* widget, QApplication::topLevelWidgets())
	{
		MainWindow* mw = qobject_cast<MainWindow*>(widget);
		if (mw!=nullptr)
		{
			mw->addModelessDialog(dlg, maximize);
		}
	}
}
const VariantList&GlobalServiceProvider::getSmallVariantList()
{
	foreach(QWidget* widget, QApplication::topLevelWidgets())
	{
		MainWindow* mw = qobject_cast<MainWindow*>(widget);
		if (mw!=nullptr)
		{
			return mw->getSmallVariantList();
		}
	}
	THROW(ProgrammingException, "MainWindow not found!");
}

const CnvList&GlobalServiceProvider::getCnvList()
{
	foreach(QWidget* widget, QApplication::topLevelWidgets())
	{
		MainWindow* mw = qobject_cast<MainWindow*>(widget);
		if (mw!=nullptr)
		{
			return mw->getCnvList();
		}
	}
	THROW(ProgrammingException, "MainWindow not found!");
}

const BedpeFile&GlobalServiceProvider::getSvList()
{
	foreach(QWidget* widget, QApplication::topLevelWidgets())
	{
		MainWindow* mw = qobject_cast<MainWindow*>(widget);
		if (mw!=nullptr)
		{
			return mw->getSvList();
		}
	}
	THROW(ProgrammingException, "MainWindow not found!");
}
