#ifndef ENDPOINTHANDLER_H
#define ENDPOINTHANDLER_H

#include <QFile>
#include <QDebug>
#include <QDir>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonDocument>

#include "Exceptions.h"
#include "FileCache.h"
#include "NGSD.h"
#include "SessionManager.h"
#include "EndpointHelper.h"
#include "UrlManager.h"

#include "FileLocationProviderLocal.h"
#include "VariantList.h"


class EndpointHandler
{


public:
    EndpointHandler();

	static HttpResponse serveIndexPage(HttpRequest request);
	static HttpResponse serveApiInfo(HttpRequest request);
	static HttpResponse serveTempUrl(HttpRequest request);
	static HttpResponse locateFileByType(HttpRequest request);
	static HttpResponse locateProjectFile(HttpRequest request);
	static HttpResponse performLogin(HttpRequest request);
	static HttpResponse performLogout(HttpRequest request);

private:
	static bool isValidUser(QString name, QString password);	
	static QList<QString> getAnalysisFiles(QString sample_name, bool search_multi);
	static QString createTempUrl(QString filename);
};

#endif // ENDPOINTHANDLER_H
