#ifndef GLOBALSERVICEPROVIDER_H
#define GLOBALSERVICEPROVIDER_H

#include <QSharedPointer>
#include "FileLocationProvider.h"
#include "DatabaseService.h"
#include "StatisticsService.h"
#include <QDialog>
#include <QMutex>

struct IGVSession
{
	int port;
	bool is_initialized;
	QString genome;
};

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
	static void executeCommandListInIGV(QStringList commands, bool init_if_not_done, int session_index);
	static void executeCommandInIGV(QString command, bool init_if_not_done, int session_index);
	static void gotoInIGV(QString region, bool init_if_not_done, int session_index = 0);
	static void loadFileInIGV(QString filename, bool init_if_not_done, bool is_virus_genome = false);
	//methods to handle dedicated IGV instances
	static int createIGVSession(int port, bool is_initialized, QString genome);
	static void removeIGVSession(int session_index);
	static int findAvailablePortForIGV();
	static int getIGVPort(int session_index);
	static void setIGVPort(int port, int session_index);
	static QString getIGVGenome(int session_index);
	static void setIGVGenome(QString genome, int session_index);
	static bool isIGVInitialized(int session_index);
	static void setIGVInitialized(bool is_initialized, int session_index);

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
	QList<IGVSession> session_list_;
	QMutex mutex_;
};

#endif // GLOBALSERVICEPROVIDER_H
