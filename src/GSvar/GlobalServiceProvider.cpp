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
  , normal_igv_port_manual_(-1)
  , virus_igv_port_manual_(-1)
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

void GlobalServiceProvider::executeCommandListInIGV(QStringList commands, bool init_if_not_done,bool is_virus_genome)
{
	foreach(QWidget* widget, QApplication::topLevelWidgets())
	{
		MainWindow* mw = qobject_cast<MainWindow*>(widget);
		if (mw!=nullptr)
		{
			mw->executeIGVCommands(commands, init_if_not_done, is_virus_genome);
		}
	}
}

void GlobalServiceProvider::executeCommandInIGV(QString command, bool init_if_not_done, bool is_virus_genome)
{
	executeCommandListInIGV(QStringList() << command, init_if_not_done, is_virus_genome);
}

void GlobalServiceProvider::gotoInIGV(QString region, bool init_if_not_done, bool is_virus_genome)
{
	executeCommandInIGV("goto " + region, init_if_not_done, is_virus_genome);
}

void GlobalServiceProvider::loadFileInIGV(QString filename, bool init_if_not_done, bool is_virus_genome)
{
	//normalize local files
	filename = Helper::canonicalPath(filename);

	if (NGSHelper::isClientServerMode()) executeCommandInIGV("SetAccessToken " + LoginManager::userToken() + " *" + Settings::string("server_host") + "*", init_if_not_done, is_virus_genome);
	executeCommandInIGV("load \"" + NGSHelper::stripSecureToken(filename) + "\"", init_if_not_done, is_virus_genome);
}

int GlobalServiceProvider::getIGVPort(bool is_virus_genome)
{
	int port = Settings::integer("igv_port");

	//if NGSD is enabled, add the user ID (like that, several users can work on one server)
	if (LoginManager::active())
	{
		port += LoginManager::userId();
	}

	//use different ranges for different genome build, so that they can be used in parallel
	if (GSvarHelper::build()!=GenomeBuild::HG19)
	{
		port += 1000;
	}

	//IGV instance created for the virus detection
	if (is_virus_genome)
	{
		port += 1000;
	}

	//if manual override is set, use it
	if ((is_virus_genome) && (instance().virus_igv_port_manual_>0))
	{
		port = instance().virus_igv_port_manual_;
	}

	if ((!is_virus_genome) && (instance().normal_igv_port_manual_>0))
	{
		port = instance().normal_igv_port_manual_;
	}

	return port;
}

void GlobalServiceProvider::setIGVPort(int port, bool is_virus_genome)
{
	if (is_virus_genome)
	{
		instance().virus_igv_port_manual_ = port;
	}
	else
	{
		instance().normal_igv_port_manual_ = port;
	}
}

bool GlobalServiceProvider::isIGVInitialized(bool is_virus_genome)
{
	if (is_virus_genome)
	{
		return instance().is_virus_igv_initialized;
	}
	else
	{
		return instance().is_normal_igv_initialized;
	}
}

void GlobalServiceProvider::setIGVInitialized(bool is_initialized, bool is_virus_genome)
{
	if (is_virus_genome)
	{
		instance().is_virus_igv_initialized = is_initialized;
	}
	else
	{
		instance().is_normal_igv_initialized = is_initialized;
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
