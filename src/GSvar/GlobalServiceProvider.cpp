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
