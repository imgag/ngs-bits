#ifndef HTTPPROCESSOR_H
#define HTTPPROCESSOR_H

#include "cppREST_global.h"
#include "HttpParts.h"

class CPPRESTSHARED_EXPORT HttpProcessor
{
public:
	static ContentType getContentTypeFromString(const QString& in);
	static QString convertContentTypeToString(const ContentType& in);
	static ContentType getContentTypeByFilename(const QString& filename);

	static RequestMethod getMethodTypeFromString(const QString& in);
	static QString convertMethodTypeToString(const RequestMethod& in);

	static QString convertResponseStatusToReasonPhrase(const ResponseStatus& response_status);
	static int convertResponseStatusToStatusCode(const ResponseStatus& status_code);

protected:
	HttpProcessor();

private:
	static HttpProcessor& instance();

};

#endif // HTTPPROCESSOR_H
