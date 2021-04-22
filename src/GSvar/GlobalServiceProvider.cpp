#include "GlobalServiceProvider.h"

#include "Settings.h"
#include "DatabaseServiceLocal.h"

GlobalServiceProvider::GlobalServiceProvider()
  : file_location_provider_()
  , database_service_()
{
	if (Settings::string("server_host",true).trimmed()!="" && Settings::string("server_port").trimmed()!="")
	{
		//TODO GSvarServer
	}
	else
	{
		database_service_ = QSharedPointer<DatabaseService>(new DatabaseServiceLocal());
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
	return *(instance().database_service_);
}
