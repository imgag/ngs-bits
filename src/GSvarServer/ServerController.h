#ifndef SERVERCONTROLLER_H
#define SERVERCONTROLLER_H

#include <QFile>
#include <QDebug>
#include <QDir>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonDocument>

#include "Log.h"
#include "Exceptions.h"
#include "EndpointController.h"
#include "FileLocationProviderLocal.h"
#include "VariantList.h"

struct SampleMetadata
{
	SampleHeaderInfo header;
	AnalysisType type;
};

class ServerController
{

public:
	ServerController();

	/// Serves files saved in the server assets
	static HttpResponse serveResourceAsset(const HttpRequest& request);
	/// Creates and returns a temporary URL for a specific file
	static HttpResponse serveTempUrl(const HttpRequest& request);
	/// Returns a location object for a file based on its type
	static HttpResponse locateFileByType(const HttpRequest& request);
	/// Returns the location of the processed sample
	static HttpResponse getProcessedSamplePath(const HttpRequest& request);
	/// Lcoates a GSvar file related to the specified job
	static HttpResponse getAnalysisJobGSvarFile(const HttpRequest& request);
	/// Saves changes to the project file
	static HttpResponse saveProjectFile(const HttpRequest& request);
	/// Saves qbic files in the folder on the server
	static HttpResponse saveQbicFiles(const HttpRequest& request);
	/// Uploads a file to the sample folder (via multipart form POST request)
	static HttpResponse uploadFile(const HttpRequest& request);
	/// Requests a secure token that is needed for the communication with the server
	static HttpResponse performLogin(const HttpRequest& request);
	/// Destoroys the user's session and invalidates the token
	static HttpResponse performLogout(const HttpRequest& request);

	/// Streams processing system regions file
	static HttpResponse getProcessingSystemRegions(const HttpRequest& request);
	/// Streams processing system amplicons file
	static HttpResponse getProcessingSystemAmplicons(const HttpRequest& request);
	/// Streams processing system genes file
	static HttpResponse getProcessingSystemGenes(const HttpRequest& request);
	/// Retrieves a list of secondary analyses
	static HttpResponse getSecondaryAnalyses(const HttpRequest& request);

private:
	/// Creates a temporary URL for a file (includes a file name and its full path)
	static QString createFileTempUrl(const QString& file, const QString& token, const bool& return_http);
};

#endif // SERVERCONTROLLER_H
