#include "GlobalServiceProvider.h"

#include "Settings.h"
#include "DatabaseServiceLocal.h"
#include "DatabaseServiceRemote.h"
#include "StatisticsServiceLocal.h"
#include "StatisticsServiceRemote.h"
#include "MainWindow.h"
#include "GSvarHelper.h"
#include "ClientHelper.h"

GlobalServiceProvider::GlobalServiceProvider()
  : file_location_provider_()
  , database_service_()
  , statistics_service_()
{
	if (ClientHelper::isClientServerMode())
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
	mainWindow()->openProcessedSampleTab(processed_sample_name);
}

void GlobalServiceProvider::openRunTab(QString run_name)
{
	mainWindow()->openRunTab(run_name);
}

void GlobalServiceProvider::openGeneTab(QString symbol)
{
	mainWindow()->openGeneTab(symbol);
}

void GlobalServiceProvider::openVariantTab(Variant variant)
{
	mainWindow()->openVariantTab(variant);
}

void GlobalServiceProvider::openProjectTab(QString project_name)
{
	mainWindow()->openProjectTab(project_name);
}

void GlobalServiceProvider::openProcessingSystemTab(QString system_short_name)
{
	mainWindow()->openProcessingSystemTab(system_short_name);
}

void GlobalServiceProvider::openGSvarViaNGSD(QString processed_sample_name, bool search_multi)
{
	mainWindow()->openProcessedSampleFromNGSD(processed_sample_name, search_multi);
}

void GlobalServiceProvider::addModelessDialog(QSharedPointer<QDialog> dlg, bool maximize)
{
	mainWindow()->addModelessDialog(dlg, maximize);
}
const VariantList& GlobalServiceProvider::getSmallVariantList()
{
	return mainWindow()->getSmallVariantList();
}

const CnvList& GlobalServiceProvider::getCnvList()
{
	return mainWindow()->getCnvList();
}

const BedpeFile& GlobalServiceProvider::getSvList()
{
	return mainWindow()->getSvList();
}

MainWindow* GlobalServiceProvider::mainWindow()
{
	foreach(QWidget* widget, QApplication::topLevelWidgets())
	{
		MainWindow* w = qobject_cast<MainWindow*>(widget);
		if (w!=nullptr) return w;
	}

	THROW(ProgrammingException, "Could not find main window!");
}
