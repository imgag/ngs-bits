#ifndef HTTPPROCESSOR_H
#define HTTPPROCESSOR_H

#include "cppREST_global.h"
#include "HttpParts.h"

class CPPRESTSHARED_EXPORT HttpProcessor
{
public:
	static ContentType getContentTypeFromString(QString in);	
	static QString convertContentTypeToString(ContentType in);
	static ContentType getContentTypeByFilename(QString filename);

	static RequestMethod getMethodTypeFromString(QString in);
	static QString convertMethodTypeToString(RequestMethod in);

	static QString convertStatusCodeToReasonPhrase(StatusCode status_code);
	static int convertStatusCodeToInt(StatusCode status_code);

protected:
	HttpProcessor();

private:
	static HttpProcessor& instance();

};

#endif // HTTPPROCESSOR_H
