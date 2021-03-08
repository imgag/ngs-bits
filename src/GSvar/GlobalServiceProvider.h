#ifndef GLOBALSERVICEPROVIDER_H
#define GLOBALSERVICEPROVIDER_H

#include<QSharedPointer>
#include "FileLocationProvider.h"

class GlobalServiceProvider
{
public:
	static GlobalServiceProvider& instance();
	static void setFileLocationProvider(QSharedPointer<FileLocationProvider> file_location_provider);
	static const QSharedPointer<FileLocationProvider> fileLocationProvider();

protected:
	GlobalServiceProvider();
	~GlobalServiceProvider();

private:
	QSharedPointer<FileLocationProvider> file_location_provider_;
};

#endif // GLOBALSERVICEPROVIDER_H
