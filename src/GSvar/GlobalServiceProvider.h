#ifndef GLOBALSERVICEPROVIDER_H
#define GLOBALSERVICEPROVIDER_H

#include<QSharedPointer>
#include "FileLocationProvider.h"


class GlobalServiceProvider
{
public:
	static GlobalServiceProvider& instance();

	static void setfileLocationsProvider(QSharedPointer<FileLocationProvider> file_location_provider);
	static const FileLocationProvider& fileLocationsProvider();

protected:
	GlobalServiceProvider();
	~GlobalServiceProvider();

private:
	QSharedPointer<FileLocationProvider> file_location_provider_;
};

#endif // GLOBALSERVICEPROVIDER_H
