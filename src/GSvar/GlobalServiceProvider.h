#ifndef GLOBALSERVICEPROVIDER_H
#define GLOBALSERVICEPROVIDER_H

#include <QSharedPointer>
#include "FileLocationProvider.h"
#include "DatabaseService.h"
#include "StatisticsService.h"
#include <QDialog>

///Provider class for GSvar-wide services
class GlobalServiceProvider
{
public:
	//analysis file location functionality (depends on where the file was opened from)
	static const FileLocationProvider& fileLocationProvider();
	static void setFileLocationProvider(QSharedPointer<FileLocationProvider> file_location_provider);
	static void clearFileLocationProvider();

	//database service functionality
	static const DatabaseService& database();

	//statistics service functionality
	static StatisticsService& statistics();

	//NGSD tab functionality
	static void openProcessedSampleTab(QString processed_sample_name);
	static void openRunTab(QString run_name);
	static void openGeneTab(QString symbol);
	static void openVariantTab(Variant variant);
	static void openProjectTab(QString project_name);
	static void openProcessingSystemTab(QString system_short_name);

	//IGV functionality
	static void gotoInIGV(QString region, bool init_if_not_done);
	static void loadFileInIGV(QString filename, bool init_if_not_done);

	//opening GSvar files
	static void openGSvarViaNGSD(QString processed_sample_name, bool search_multi);

	//add modeless dialog
	static void addModelessDialog(QSharedPointer<QDialog> dlg, bool maximize=false);
	//get sample variants
	static const VariantList& getSmallVariantList();
	static const CnvList& getCnvList();
	static const BedpeFile& getSvList();
protected:
	GlobalServiceProvider();
	~GlobalServiceProvider();
	static GlobalServiceProvider& instance();

private:
	QSharedPointer<FileLocationProvider> file_location_provider_;
	QSharedPointer<DatabaseService> database_service_;
	QSharedPointer<StatisticsService> statistics_service_;
};

#endif // GLOBALSERVICEPROVIDER_H
