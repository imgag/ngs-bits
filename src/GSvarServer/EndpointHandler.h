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

struct SampleMetadata
{
	SampleHeaderInfo header;
	AnalysisType type;
};


class EndpointHandler
{


public:
    EndpointHandler();

	static HttpResponse serveIndexPage(const HttpRequest& request);
	static HttpResponse serveFavicon(const HttpRequest& request);
	static HttpResponse serveApiInfo(const HttpRequest& request);
	static HttpResponse serveTempUrl(const HttpRequest& request);
	static HttpResponse locateFileByType(const HttpRequest& request);
	static HttpResponse getProcessedSamplePath(const HttpRequest& request);
	static HttpResponse saveProjectFile(const HttpRequest& request);
	static HttpResponse saveQbicFiles(const HttpRequest& request);

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
	static QString createFileTempUrl(const QString& file, const bool& return_http);
	/// Reads the sample header from a variant list
	static SampleMetadata getSampleMetadata(const QString& gsvar_file, bool allow_fallback_germline_single_sample=true);
	/// Finds a column index by the annotation name (column title)
	static int annotationIndexByName(QList<VariantAnnotationHeader>& annotations, const QString& name, bool exact_match, bool error_on_mismatch);

};

#endif // ENDPOINTHANDLER_H