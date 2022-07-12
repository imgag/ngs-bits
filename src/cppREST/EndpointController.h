#ifndef ENDPOINTCONTROLLER_H
#define ENDPOINTCONTROLLER_H

#include "cppREST_global.h"
#include "Log.h"
#include "HttpParts.h"
#include "EndpointManager.h"
#include "UrlManager.h"
#include "HttpResponse.h"
#include "SessionManager.h"
#include <QUrl>
#include <QDir>


class CPPRESTSHARED_EXPORT EndpointController
{
public:	
	/// Shows a page explaining what a particular(s) endpoint(s) does(do)
	static HttpResponse serveEndpointHelp(const HttpRequest& request);
	/// Provides a random access to a file or streams it (depending on the headers), as well as displays a folder
	/// content from the server root folder
	static HttpResponse serveStaticFromServerRoot(const HttpRequest& request);
	/// Provides a random access to a file or streams it (depending on the headers), as well as displays a folder
	/// content for a specific project folder linked to a temporary URL
	static HttpResponse serveStaticForTempUrl(const HttpRequest& request);

	/// Serves a file for a byte range request (i.e. specific fragment of a file)
	static HttpResponse createStaticFileRangeResponse(QString filename, QList<ByteRange> byte_ranges, ContentType type, bool is_downloadable);
	/// Serves a stream, used to transfer files
	static HttpResponse createStaticStreamResponse(QString filename, bool is_downloadable);
	/// Serves a file from the server cache (not fully implementd and not used yet)
	static HttpResponse createStaticFromCacheResponse(QString id, QList<ByteRange> byte_ranges, ContentType type, bool is_downloadable);
	/// Handles all requests for static data
	static HttpResponse serveStaticFile(QString filename, RequestMethod method, ContentType content_type, QMap<QString, QList<QString>> headers);

protected:
	EndpointController();

private:
	static EndpointController& instance();

	static HttpResponse serveFolderContent(const QString path, const HttpRequest& request);
	static HttpResponse serveFolderListing(QString folder_title, QString cur_folder_url, QString parent_folder_url, QList<FolderItem> items, QString token);

	static QString getEndpointHelpTemplate(QList<Endpoint> endpoint_list);
	static QString generateHelpPage();
	static QString generateHelpPage(const QString& path, const RequestMethod& method);
	static QString generateHelpPage(const QString& path);
	static QString getServedTempPath(QList<QString> path_parts);
	static QString getServedRootPath(const QList<QString>& path_parts);
	static bool hasOverlappingRanges(const QList<ByteRange> ranges);
};

#endif // ENDPOINTCONTROLLER_H
