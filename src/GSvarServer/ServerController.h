#ifndef SERVERCONTROLLER_H
#define SERVERCONTROLLER_H


#include "Log.h"
#include "VariantList.h"
#include "HttpResponse.h"
#include "HttpRequest.h"
#include "EndpointManager.h"
#include "FastFileInfo.h"


struct SampleMetadata
{
	SampleHeaderInfo header;
	AnalysisType type;
};

class ServerController
{

public:
	ServerController();
	/// Shows a page explaining what a particular(s) endpoint(s) does(do)
	static HttpResponse serveEndpointHelp(const HttpRequest& request);
	/// Provides a random access to a file or streams it (depending on the headers), as well as displays a folder
	/// content from the server root folder
	static HttpResponse serveStaticFromServerRoot(const HttpRequest& request);
	/// Returns a genome saved on the server
	static HttpResponse serveStaticServerGenomes(const HttpRequest& request);
	/// Provides a random access to a file or streams it (depending on the headers), as well as displays a folder
	/// content for a specific project folder linked to a temporary URL
	static HttpResponse serveStaticFromTempUrl(const HttpRequest& request);
	/// Serves files saved in the server assets
	static HttpResponse serveResourceAsset(const HttpRequest& request);
	/// Creates and returns a temporary URL for a specific file
	static HttpResponse serveTempUrl(const HttpRequest& request);
	/// Returns a location object for a file based on its type
	static HttpResponse locateFileByType(const HttpRequest& request);
	/// Returns the location of the processed sample
    static HttpResponse getProcessedSamplePath(const HttpRequest& request);
    /// Returns the location id (hash) of the processed sample
    static HttpResponse getProcessedSampleHash(const HttpRequest& request);
	/// Locates a GSvar file related to the specified job
	static HttpResponse getAnalysisJobGSvarFile(const HttpRequest& request);
	/// Date and time of the last modification of a log file for a specific job
	static HttpResponse getAnalysisJobLastUpdate(const HttpRequest& request);
	/// Locates a log file for a specific job
	static HttpResponse getAnalysisJobLog(const HttpRequest& request);
	/// Saves changes to the project file
	static HttpResponse saveProjectFile(const HttpRequest& request);
	/// Saves qbic files in the folder on the server
	static HttpResponse saveQbicFiles(const HttpRequest& request);
	/// Uploads a file to the sample folder (via multipart form POST request)
	static HttpResponse uploadFile(const HttpRequest& request);
	/// Uploads a VCF file, annotates it, and converts into GSvar
	static HttpResponse annotateVariant(const HttpRequest& request);
	/// Starts the calculation of low coverage regions
	static HttpResponse calculateLowCoverage(const HttpRequest& request);
	/// Starts the calculation of the average covarage for gaps
	static HttpResponse calculateAvgCoverage(const HttpRequest& request);
	/// Calculates target region read depth used in germline report
    static HttpResponse calculateTargetRegionReadDepth(const HttpRequest& request);
	/// Creates a list of analysis names for multi-samples
	static HttpResponse getMultiSampleAnalysisInfo(const HttpRequest& request);
	/// Requests a secure token that is needed for the communication with the server
	static HttpResponse performLogin(const HttpRequest& request);
	/// Returns information about the session (i.g. login time, user id, for how log it is valid, etc.)
	static HttpResponse getSessionInfo(const HttpRequest& request);
	/// Checks if prodivided login and password are valid
	static HttpResponse validateCredentials(const HttpRequest& request);
	/// Requests a toke to access the database credentials
	static HttpResponse getDbToken(const HttpRequest& request);
	/// Requests NGSD credentials for the GSvar application
	static HttpResponse getNgsdCredentials(const HttpRequest& request);
	/// Requests Genlab database credentials for the GSvar application
	static HttpResponse getGenlabCredentials(const HttpRequest& request);
	/// Destoroys the user's session and invalidates the token
	static HttpResponse performLogout(const HttpRequest& request);
	/// Streams processing system regions file
	static HttpResponse getProcessingSystemRegions(const HttpRequest& request);
	/// Streams processing system genes file
	static HttpResponse getProcessingSystemGenes(const HttpRequest& request);
	/// Retrieves a list of secondary analyses
	static HttpResponse getSecondaryAnalyses(const HttpRequest& request);
	/// Returns a list RNA fusion plots needed for a report
	static HttpResponse getRnaFusionPics(const HttpRequest& request);
	/// Returns a list RNA fusion plots needed for a report
	static HttpResponse getRnaExpressionPlots(const HttpRequest& request);
	/// Returns information about the latest available version of the desktop client
	static HttpResponse getCurrentClientInfo(const HttpRequest& request);

    /// Returns BLAT search results for the given genome and sequence
    static HttpResponse performBlatSearch(const HttpRequest& request);

	// Returns some notification displayed to the users of the client application
	static HttpResponse getCurrentNotification(const HttpRequest& request);

    static HttpResponse uploadFileToFolder(QString upload_folder, const HttpRequest& request);
private:
	/// Find file/folder name corresponding to the id from a temporary URL
	static QString findPathForTempUrl(QList<QString> path_parts);
	/// Find file/folder name corresponding to the server root data
	static QString findPathForServerFolder(const QList<QString>& path_parts, QString server_folder);
	/// Check if byte-range request contains overlapping ranges, they are
	/// not allowed, according to the HTTP specification
    static bool hasOverlappingRanges(const QList<ByteRange>& ranges);
    /// Finds filename with full path for a given processed sample
    static QString getProcessedSampleFile(const int& ps_id, const PathType& type, const QString& token);
    /// Returns a temporary URL for a file
	static QString createTempUrl(const QString& file, const QString& token);
    static QString createTempUrl(FastFileInfo& file_info, const QString& token);
    /// Returns a temporary URL wihtout a parameters (e.g. ?token=123)
    static QString stripParamsFromTempUrl(const QString& url);

	/// Serves a file for a byte range request (i.e. specific fragment of a file)
	static HttpResponse createStaticFileRangeResponse(const QString& filename, const QList<ByteRange>& byte_ranges, const ContentType& type, bool is_downloadable);
	/// Serves a stream, used to transfer large files without opening multiple connections
	static HttpResponse createStaticStreamResponse(const QString& filename, bool is_downloadable);
    static HttpResponse createStaticFileResponse(const QString& filename, const HttpRequest& request);
	static HttpResponse createStaticFolderResponse(const QString path, const HttpRequest& request);
	static HttpResponse createStaticLocationResponse(const QString path, const HttpRequest& request);
};

#endif // SERVERCONTROLLER_H
