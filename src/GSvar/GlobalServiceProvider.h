#ifndef GLOBALSERVICEPROVIDER_H
#define GLOBALSERVICEPROVIDER_H

#include<QSharedPointer>
#include "FileLocationProvider.h"
#include "DatabaseService.h"

class GlobalServiceProvider
{
public:
	//analysis file location functionality (depends on where the file was opened from)
	static const FileLocationProvider& fileLocationProvider();
	static void setFileLocationProvider(QSharedPointer<FileLocationProvider> file_location_provider);
	static void clearFileLocationProvider();

	//database service functionality
	static const DatabaseService& database();

protected:
	GlobalServiceProvider();
	~GlobalServiceProvider();
	static GlobalServiceProvider& instance();

private:
	QSharedPointer<FileLocationProvider> file_location_provider_;
	QSharedPointer<DatabaseService> database_service_;
};

#endif // GLOBALSERVICEPROVIDER_H
