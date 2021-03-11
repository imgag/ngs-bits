#include <stdio.h>
#include <stddef.h>
#include <iostream>
#include "GlobalServiceProvider.h"


GlobalServiceProvider::GlobalServiceProvider()
  :  file_location_provider_()
{
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

const QSharedPointer<FileLocationProvider> GlobalServiceProvider::fileLocationProvider()
{	
	if (instance().file_location_provider_.isNull())
	{
		THROW(ProgrammingException, "File location provider requested but not set!");
	}

	return instance().file_location_provider_;
}
