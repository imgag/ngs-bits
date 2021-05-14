#ifndef ENDPOINTHANDLER_H
#define ENDPOINTHANDLER_H

#include <QFile>
#include <QDebug>
#include <QDir>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonDocument>

#include "Exceptions.h"
#include "EndpointController.h"
#include "FileLocationProviderLocal.h"
#include "VariantList.h"


class EndpointHandler
{


public:
    EndpointHandler();

	static HttpResponse serveIndexPage(HttpRequest request);
	static HttpResponse serveFavicon(HttpRequest request);
	static HttpResponse serveApiInfo(HttpRequest request);
	static HttpResponse serveTempUrl(HttpRequest request);
	static HttpResponse locateFileByType(HttpRequest request);
	static HttpResponse locateProjectFile(HttpRequest request);

	/// Streams processing system regions file
	static HttpResponse getProcessingSystemRegions(HttpRequest request);
	/// Streams processing system amplicons file
	static HttpResponse getProcessingSystemAmplicons(HttpRequest request);
	/// Streams processing system genes file
	static HttpResponse getProcessingSystemGenes(HttpRequest request);

	static HttpResponse performLogin(HttpRequest request);
	static HttpResponse performLogout(HttpRequest request);

private:
	static bool isValidUser(QString name, QString password);	
	static QList<QString> getAnalysisFiles(QString sample_name, bool search_multi);

	/// Creates a temporary URL for a file (includes a file name and its full path)
	static QString createFileTempUrl(QString file);
};

#endif // ENDPOINTHANDLER_H
