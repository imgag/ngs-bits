#include "HttpProcessor.h"

HttpProcessor::HttpProcessor()
{
}

HttpProcessor& HttpProcessor::instance()
{
	static HttpProcessor http_processor;
	return http_processor;
}

ContentType HttpProcessor::getContentTypeFromString(const QString& in)
{
	if (in.toLower() == "application/octet-stream") return APPLICATION_OCTET_STREAM;
	if (in.toLower() == "application/json") return APPLICATION_JSON;
	if (in.toLower() == "application/javascript") return APPLICATION_JAVASCRIPT;
	if (in.toLower() == "image/jpeg") return IMAGE_JPEG;
	if (in.toLower() == "image/png") return IMAGE_PNG;
	if (in.toLower() == "image/svg+xml") return IMAGE_SVG_XML;
	if (in.toLower() == "text/plain") return TEXT_PLAIN;
	if (in.toLower() == "text/csv") return TEXT_CSV;
	if (in.toLower() == "text/html") return TEXT_HTML;
	if (in.toLower() == "text/xml") return TEXT_XML;
	if (in.toLower() == "text/css") return TEXT_CSS;
	if (in.toLower() == "multipart/form-data") return MULTIPART_FORM_DATA;
	if (in.toLower() == "application/x-www-form-urlencoded") return APPLICATION_X_WWW_FORM_URLENCODED;

	return APPLICATION_OCTET_STREAM;
}

QString HttpProcessor::convertContentTypeToString(const ContentType& in)
{
	switch(in)
	{
		case APPLICATION_OCTET_STREAM: return "application/octet-stream";
		case APPLICATION_JSON: return "application/json";
		case APPLICATION_JAVASCRIPT: return "application/javascript";
		case IMAGE_JPEG: return "image/jpeg";
		case IMAGE_PNG: return "image/png";
		case IMAGE_SVG_XML: return "image/svg+xml";
		case TEXT_PLAIN: return "text/plain";
		case TEXT_CSV: return "text/csv";
		case TEXT_HTML: return "text/html";
		case TEXT_XML: return "text/xml";
		case TEXT_CSS: return "text/css";
		case MULTIPART_FORM_DATA: return "multipart/form-data";
		case APPLICATION_X_WWW_FORM_URLENCODED: return "application/x-www-form-urlencoded";
	}
	return "";
}

ContentType HttpProcessor::getContentTypeByFilename(const QString& filename)
{
	QList<QString> name_items = filename.split(".");
	QString extention = name_items.takeLast().toLower();

	if (extention == "json") return APPLICATION_JSON;
	if (extention == "js") return APPLICATION_JAVASCRIPT;
	if ((extention == "jpeg") || (extention == "jpg")) return IMAGE_JPEG;
	if (extention == "png") return IMAGE_PNG;
	if (extention == "svg") return IMAGE_SVG_XML;
	if (extention == "txt") return TEXT_PLAIN;
	if (extention == "csv") return TEXT_CSV;
	if ((extention == "html") || (extention == "htm")) return TEXT_HTML;
	if (extention == "xml") return TEXT_XML;
	if (extention == "css") return TEXT_CSS;
	if (extention == "bam") return APPLICATION_OCTET_STREAM;
	if (extention == "bai") return APPLICATION_OCTET_STREAM;
	if (extention == "gsvar") return TEXT_PLAIN;
	if (extention == "bed") return TEXT_PLAIN;
	if (extention == "tsv") return TEXT_PLAIN;
	if (extention == "seg") return TEXT_PLAIN;
	if (extention == "igv") return TEXT_PLAIN;
	if (extention == "gz") return APPLICATION_OCTET_STREAM;
	if (extention == "vcf") return TEXT_PLAIN;

	return APPLICATION_OCTET_STREAM;
}

RequestMethod HttpProcessor::getMethodTypeFromString(const QString& in)
{
	if (in.toLower() == "get") return RequestMethod::GET;
	if (in.toLower() == "post") return RequestMethod::POST;
	if (in.toLower() == "delete") return RequestMethod::DELETE;
	if (in.toLower() == "put") return RequestMethod::PUT;
	if (in.toLower() == "patch") return RequestMethod::PATCH;
	if (in.toLower() == "head") return RequestMethod::HEAD;

	return RequestMethod::GET;
}

QString HttpProcessor::convertMethodTypeToString(const RequestMethod& in)
{
	switch(in)
	{
		case RequestMethod::GET: return "get";
		case RequestMethod::POST: return "post";
		case RequestMethod::DELETE: return "delete";
		case RequestMethod::PUT: return "put";
		case RequestMethod::PATCH: return "patch";
		case RequestMethod::HEAD: return "head";
		default: return "unknown";
	}
}

QString HttpProcessor::convertResponseStatusToReasonPhrase(const ResponseStatus& response_status)
{
	switch(response_status)
	{
		case CONTINUE: return "Continue";
		case SWITCHING_PROTOCOLS: return "Switching Protocols";
		case OK: return "OK";
		case CREATED: return "Created";
		case ACCEPTED: return "Accepted";
		case NON_AUTHORITATIVE_INFORMATION: return "Non-Authoritative Information";
		case NO_CONTENT: return "No Content";
		case RESET_CONTENT: return "Reset Content";
		case PARTIAL_CONTENT: return "Partial Content";
		case MULTIPLE_CHOICES: return "Multiple Choices";
		case MOVED_PERMANENTLY: return "Moved Permanently";
		case FOUND: return "Found";
		case SEE_OTHER: return "See Other";
		case NOT_MODIFIED: return "Not Modified";
		case USE_PROXY: return "Use Proxy";
		case TEMPORARY_REDIRECT: return "Temporary Redirect";
		case BAD_REQUEST: return "Bad Request";
		case UNAUTHORIZED: return "Unauthorized";
		case PAYMENT_REQUIRED: return "Payment Required";
		case FORBIDDEN: return "Forbidden";
		case NOT_FOUND: return "Not Found";
		case METHOD_NOT_ALLOWED: return "Method Not Allowed";
		case NOT_ACCEPTABLE: return "Not Acceptable";
		case PROXY_AUTH_REQUIRED: return "Proxy Authentication Required";
		case REQUEST_TIMEOUT: return "Request Timeout";
		case CONFLICT: return "Conflict";
		case GONE: return "Gone";
		case LENGTH_REQUIRED: return "Length Required";
		case PRECONDITION_FAILED: return "Precondition Failed";
		case ENTITY_TOO_LARGE: return "Request Entity Too Large";
		case URI_TOO_LONG: return "Request-URI Too Long";
		case UNSUPPORTED_MEDIA_TYPE: return "Unsupported Media Type";
		case RANGE_NOT_SATISFIABLE: return "Requested Range Not Satisfiable";
		case EXPECTATION_FAILED: return "Expectation Failed";
		case INTERNAL_SERVER_ERROR: return "Internal Server Error";
		case NOT_IMPLEMENTED: return "Not Implemented";
		case BAD_GATEWAY: return "Bad Gateway";
		case SERVICE_UNAVAILABLE: return "Service Unavailable";
		case GATEWAY_TIMEOUT: return "Gateway Timeout";
		case HTTP_VERSION_NOT_SUPPORTED: return "HTTP Version Not Supported";
		case UNKNOWN_STATUS_CODE:
		default: return "Unknown Status Code";
	}
}

int HttpProcessor::convertResponseStatusToStatusCodeNumber(const ResponseStatus& response_status)
{
	switch(response_status)
	{
		case CONTINUE: return 100;
		case SWITCHING_PROTOCOLS: return 101;
		case OK: return 200;
		case CREATED: return 201;
		case ACCEPTED: return 202;
		case NON_AUTHORITATIVE_INFORMATION: return 203;
		case NO_CONTENT: return 204;
		case RESET_CONTENT: return 205;
		case PARTIAL_CONTENT: return 206;
		case MULTIPLE_CHOICES: return 300;
		case MOVED_PERMANENTLY: return 301;
		case FOUND: return 302;
		case SEE_OTHER: return 303;
		case NOT_MODIFIED: return 304;
		case USE_PROXY: return 305;
		case TEMPORARY_REDIRECT: return 307;
		case BAD_REQUEST: return 400;
		case UNAUTHORIZED: return 401;
		case PAYMENT_REQUIRED: return 402;
		case FORBIDDEN: return 403;
		case NOT_FOUND: return 404;
		case METHOD_NOT_ALLOWED: return 405;
		case NOT_ACCEPTABLE: return 406;
		case PROXY_AUTH_REQUIRED: return 407;
		case REQUEST_TIMEOUT: return 408;
		case CONFLICT: return 409;
		case GONE: return 410;
		case LENGTH_REQUIRED: return 411;
		case PRECONDITION_FAILED: return 412;
		case ENTITY_TOO_LARGE: return 413;
		case URI_TOO_LONG: return 414;
		case UNSUPPORTED_MEDIA_TYPE: return 415;
		case RANGE_NOT_SATISFIABLE: return 416;
		case EXPECTATION_FAILED: return 417;
		case INTERNAL_SERVER_ERROR: return 500;
		case NOT_IMPLEMENTED: return 501;
		case BAD_GATEWAY: return 502;
		case SERVICE_UNAVAILABLE: return 503;
		case GATEWAY_TIMEOUT: return 504;
		case HTTP_VERSION_NOT_SUPPORTED: return 505;
		case UNKNOWN_STATUS_CODE:
		default: return 0;
	}
}

QString HttpProcessor::convertResponseStatusCodeNumberToStatusClass(const int& status_code_number)
{
	if ((status_code_number >= 100) && (status_code_number <= 199))
	{
		return "Informational response";
	}

	if ((status_code_number >= 200) && (status_code_number <= 299))
	{
		return "Successful response";
	}

	if ((status_code_number >= 300) && (status_code_number <= 399))
	{
		return "Redirect";
	}

	if ((status_code_number >= 400) && (status_code_number <= 499))
	{
		return "Client error";
	}

	if ((status_code_number >= 500) && (status_code_number <= 599))
	{
		return "Server error";
	}

	return "Unknown response class";
}

ContentType HttpProcessor::detectErrorContentType(const QList<QString> headers)
{
	ContentType error_type = ContentType::TEXT_PLAIN;
	for (int i = 0; i < headers.length(); i++)
	{
		if ((headers[i].toLower().contains("mozilla")) || (headers[i].toLower().contains("chrome")) || (headers[i].toLower().contains("edge") || (headers[i].toLower().contains("opera"))))
		{
			return ContentType::TEXT_HTML;
		}
	}
	return error_type;
}

