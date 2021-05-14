#ifndef ENDPOINTCONTROLLER_H
#define ENDPOINTCONTROLLER_H

#include "cppREST_global.h"
#include "HttpParts.h"
#include "HtmlEngine.h"
#include "SessionManager.h"
#include "FileCache.h"
#include "EndpointManager.h"
#include "UrlManager.h"
#include "HttpResponse.h"
#include "NGSD.h"
#include <QUrl>


class CPPRESTSHARED_EXPORT EndpointController
{
public:	
	/// Shows a page explaining what a particular(s) endpoint(s) does(do)
	static HttpResponse serveEndpointHelp(HttpRequest request);
	/// Provides a random access to a file or streams it (depending on the headers), as well as displays a folder
	/// content from the server root folder
	static HttpResponse serveStaticFromServerRoot(HttpRequest request);
	/// Provides a random access to a file or streams it (depending on the headers), as well as displays a folder
	/// content for a specific project folder linked to a temporary URL
	static HttpResponse serveStaticForTempUrl(HttpRequest request);
	/// Serves or streams file content saved in the server cache
	static HttpResponse serveStaticFileFromCache(HttpRequest request);
	/// Provides an access to a file for authenticated users
	static HttpResponse serveProtectedStaticFile(HttpRequest request);
	/// Returns file information in JSON format for a specific file
	static HttpResponse getFileInfo(HttpRequest request);

	static HttpResponse createStaticFileResponse(QString filename, ByteRange byte_range, ContentType type, bool is_downloadable);
	static HttpResponse createStaticStreamResponse(QString filename, bool is_downloadable);
	static HttpResponse createStaticFromCacheResponse(QString id, ByteRange byte_range, ContentType type, bool is_downloadable);
	static HttpResponse serveStaticFile(QString filename, RequestMethod method, QMap<QString, QString> headers);

protected:
	EndpointController();

private:
	static EndpointController& instance();

	static HttpResponse serveFolderContent(QString path, QString request_prefix, QString request_path, QList<QString> request_path_params);
	static HttpResponse serveFolderListing(QString folder_title, QString cur_folder_url, QString parent_folder_url, QList<FolderItem> items);

	static QString getEndpointHelpTemplate(QList<Endpoint>* endpoint_list);
	static QString generateGlobalHelp();
	static QString generateEntityHelp(QString path, RequestMethod method);
	static QString getServedTempPath(QList<QString> path_parts);
	static QString getServedRootPath(QList<QString> path_parts);

	static bool isEligibileToAccess(HttpRequest request);
	static StaticFile readFileContent(QString filename, ByteRange byte_range);
	static QString addFileToCache(QString filename);



};

#endif // ENDPOINTCONTROLLER_H
