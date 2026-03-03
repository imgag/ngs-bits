#include "HttpUtils.h"

HttpUtils::HttpUtils()
{
}

HttpUtils& HttpUtils::instance()
{
	static HttpUtils http_utils;
	return http_utils;
}

ContentType HttpUtils::getContentTypeFromString(const QString& in)
{
    if (in.toLower() == "application/octet-stream") return ContentType::APPLICATION_OCTET_STREAM;
    if (in.toLower() == "application/json") return ContentType::APPLICATION_JSON;
    if (in.toLower() == "application/javascript") return ContentType::APPLICATION_JAVASCRIPT;
    if (in.toLower() == "image/jpeg") return ContentType::IMAGE_JPEG;
    if (in.toLower() == "image/png") return ContentType::IMAGE_PNG;
    if (in.toLower() == "image/svg+xml") return ContentType::IMAGE_SVG_XML;
    if (in.toLower() == "text/plain") return ContentType::TEXT_PLAIN;
    if (in.toLower() == "text/csv") return ContentType::TEXT_CSV;
    if (in.toLower() == "text/html") return ContentType::TEXT_HTML;
    if (in.toLower() == "text/xml") return ContentType::TEXT_XML;
    if (in.toLower() == "text/css") return ContentType::TEXT_CSS;
    if (in.toLower() == "multipart/form-data") return ContentType::MULTIPART_FORM_DATA;
    if (in.toLower() == "application/x-www-form-urlencoded") return ContentType::APPLICATION_X_WWW_FORM_URLENCODED;

    return ContentType::APPLICATION_OCTET_STREAM;
}

QString HttpUtils::convertContentTypeToString(const ContentType& in)
{
	switch(in)
	{
        case ContentType::APPLICATION_OCTET_STREAM: return "application/octet-stream";
        case ContentType::APPLICATION_JSON: return "application/json";
        case ContentType::APPLICATION_JAVASCRIPT: return "application/javascript";
        case ContentType::IMAGE_JPEG: return "image/jpeg";
        case ContentType::IMAGE_PNG: return "image/png";
        case ContentType::IMAGE_SVG_XML: return "image/svg+xml";
        case ContentType::TEXT_PLAIN: return "text/plain";
        case ContentType::TEXT_CSV: return "text/csv";
        case ContentType::TEXT_HTML: return "text/html";
        case ContentType::TEXT_XML: return "text/xml";
        case ContentType::TEXT_CSS: return "text/css";
        case ContentType::MULTIPART_FORM_DATA: return "multipart/form-data";
        case ContentType::APPLICATION_X_WWW_FORM_URLENCODED: return "application/x-www-form-urlencoded";
	}
	return "";
}

ContentType HttpUtils::getContentTypeByFilename(const QString& filename)
{
	QList<QString> name_items = filename.split(".");
	QString extention = name_items.takeLast().toLower();

    if (extention == "json") return ContentType::APPLICATION_JSON;
    if (extention == "js") return ContentType::APPLICATION_JAVASCRIPT;
    if ((extention == "jpeg") || (extention == "jpg")) return ContentType::IMAGE_JPEG;
    if (extention == "png") return ContentType::IMAGE_PNG;
    if (extention == "svg") return ContentType::IMAGE_SVG_XML;
    if (extention == "txt") return ContentType::TEXT_PLAIN;
    if (extention == "csv") return ContentType::TEXT_CSV;
    if ((extention == "html") || (extention == "htm")) return ContentType::TEXT_HTML;
    if (extention == "xml") return ContentType::TEXT_XML;
    if (extention == "css") return ContentType::TEXT_CSS;
    if (extention == "bam") return ContentType::APPLICATION_OCTET_STREAM;
    if (extention == "bai") return ContentType::APPLICATION_OCTET_STREAM;
    if (extention == "gsvar") return ContentType::TEXT_PLAIN;
    if (extention == "bed") return ContentType::TEXT_PLAIN;
    if (extention == "tsv") return ContentType::TEXT_PLAIN;
    if (extention == "seg") return ContentType::TEXT_PLAIN;
    if (extention == "igv") return ContentType::TEXT_PLAIN;
    if (extention == "gz") return ContentType::APPLICATION_OCTET_STREAM;
    if (extention == "vcf") return ContentType::TEXT_PLAIN;

    return ContentType::APPLICATION_OCTET_STREAM;
}

RequestMethod HttpUtils::getMethodTypeFromString(const QString& in)
{
	if (in.toLower() == "get") return RequestMethod::GET;
	if (in.toLower() == "post") return RequestMethod::POST;
	if (in.toLower() == "delete") return RequestMethod::DELETE;
	if (in.toLower() == "put") return RequestMethod::PUT;
	if (in.toLower() == "patch") return RequestMethod::PATCH;
	if (in.toLower() == "head") return RequestMethod::HEAD;

	return RequestMethod::GET;
}

QString HttpUtils::convertMethodTypeToString(const RequestMethod& in)
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

QString HttpUtils::convertResponseStatusToReasonPhrase(const ResponseStatus& response_status)
{
	switch(response_status)
	{
        case ResponseStatus::CONTINUE: return "Continue";
        case ResponseStatus::SWITCHING_PROTOCOLS: return "Switching Protocols";
        case ResponseStatus::OK: return "OK";
        case ResponseStatus::CREATED: return "Created";
        case ResponseStatus::ACCEPTED: return "Accepted";
        case ResponseStatus::NON_AUTHORITATIVE_INFORMATION: return "Non-Authoritative Information";
        case ResponseStatus::NO_CONTENT: return "No Content";
        case ResponseStatus::RESET_CONTENT: return "Reset Content";
        case ResponseStatus::PARTIAL_CONTENT: return "Partial Content";
        case ResponseStatus::MULTIPLE_CHOICES: return "Multiple Choices";
        case ResponseStatus::MOVED_PERMANENTLY: return "Moved Permanently";
        case ResponseStatus::FOUND: return "Found";
        case ResponseStatus::SEE_OTHER: return "See Other";
        case ResponseStatus::NOT_MODIFIED: return "Not Modified";
        case ResponseStatus::USE_PROXY: return "Use Proxy";
        case ResponseStatus::TEMPORARY_REDIRECT: return "Temporary Redirect";
        case ResponseStatus::BAD_REQUEST: return "Bad Request";
        case ResponseStatus::UNAUTHORIZED: return "Unauthorized";
        case ResponseStatus::PAYMENT_REQUIRED: return "Payment Required";
        case ResponseStatus::FORBIDDEN: return "Forbidden";
        case ResponseStatus::NOT_FOUND: return "Not Found";
        case ResponseStatus::METHOD_NOT_ALLOWED: return "Method Not Allowed";
        case ResponseStatus::NOT_ACCEPTABLE: return "Not Acceptable";
        case ResponseStatus::PROXY_AUTH_REQUIRED: return "Proxy Authentication Required";
        case ResponseStatus::REQUEST_TIMEOUT: return "Request Timeout";
        case ResponseStatus::CONFLICT: return "Conflict";
        case ResponseStatus::GONE: return "Gone";
        case ResponseStatus::LENGTH_REQUIRED: return "Length Required";
        case ResponseStatus::PRECONDITION_FAILED: return "Precondition Failed";
        case ResponseStatus::ENTITY_TOO_LARGE: return "Request Entity Too Large";
        case ResponseStatus::URI_TOO_LONG: return "Request-URI Too Long";
        case ResponseStatus::UNSUPPORTED_MEDIA_TYPE: return "Unsupported Media Type";
        case ResponseStatus::RANGE_NOT_SATISFIABLE: return "Requested Range Not Satisfiable";
        case ResponseStatus::EXPECTATION_FAILED: return "Expectation Failed";
        case ResponseStatus::INTERNAL_SERVER_ERROR: return "Internal Server Error";
        case ResponseStatus::NOT_IMPLEMENTED: return "Not Implemented";
        case ResponseStatus::BAD_GATEWAY: return "Bad Gateway";
        case ResponseStatus::SERVICE_UNAVAILABLE: return "Service Unavailable";
        case ResponseStatus::GATEWAY_TIMEOUT: return "Gateway Timeout";
        case ResponseStatus::HTTP_VERSION_NOT_SUPPORTED: return "HTTP Version Not Supported";
        case ResponseStatus::UNKNOWN_STATUS_CODE:
		default: return "Unknown Status Code";
	}
}

int HttpUtils::convertResponseStatusToStatusCodeNumber(const ResponseStatus& response_status)
{
	switch(response_status)
	{
        case ResponseStatus::CONTINUE: return 100;
        case ResponseStatus::SWITCHING_PROTOCOLS: return 101;
        case ResponseStatus::OK: return 200;
        case ResponseStatus::CREATED: return 201;
        case ResponseStatus::ACCEPTED: return 202;
        case ResponseStatus::NON_AUTHORITATIVE_INFORMATION: return 203;
        case ResponseStatus::NO_CONTENT: return 204;
        case ResponseStatus::RESET_CONTENT: return 205;
        case ResponseStatus::PARTIAL_CONTENT: return 206;
        case ResponseStatus::MULTIPLE_CHOICES: return 300;
        case ResponseStatus::MOVED_PERMANENTLY: return 301;
        case ResponseStatus::FOUND: return 302;
        case ResponseStatus::SEE_OTHER: return 303;
        case ResponseStatus::NOT_MODIFIED: return 304;
        case ResponseStatus::USE_PROXY: return 305;
        case ResponseStatus::TEMPORARY_REDIRECT: return 307;
        case ResponseStatus::BAD_REQUEST: return 400;
        case ResponseStatus::UNAUTHORIZED: return 401;
        case ResponseStatus::PAYMENT_REQUIRED: return 402;
        case ResponseStatus::FORBIDDEN: return 403;
        case ResponseStatus::NOT_FOUND: return 404;
        case ResponseStatus::METHOD_NOT_ALLOWED: return 405;
        case ResponseStatus::NOT_ACCEPTABLE: return 406;
        case ResponseStatus::PROXY_AUTH_REQUIRED: return 407;
        case ResponseStatus::REQUEST_TIMEOUT: return 408;
        case ResponseStatus::CONFLICT: return 409;
        case ResponseStatus::GONE: return 410;
        case ResponseStatus::LENGTH_REQUIRED: return 411;
        case ResponseStatus::PRECONDITION_FAILED: return 412;
        case ResponseStatus::ENTITY_TOO_LARGE: return 413;
        case ResponseStatus::URI_TOO_LONG: return 414;
        case ResponseStatus::UNSUPPORTED_MEDIA_TYPE: return 415;
        case ResponseStatus::RANGE_NOT_SATISFIABLE: return 416;
        case ResponseStatus::EXPECTATION_FAILED: return 417;
        case ResponseStatus::INTERNAL_SERVER_ERROR: return 500;
        case ResponseStatus::NOT_IMPLEMENTED: return 501;
        case ResponseStatus::BAD_GATEWAY: return 502;
        case ResponseStatus::SERVICE_UNAVAILABLE: return 503;
        case ResponseStatus::GATEWAY_TIMEOUT: return 504;
        case ResponseStatus::HTTP_VERSION_NOT_SUPPORTED: return 505;
        case ResponseStatus::UNKNOWN_STATUS_CODE:
		default: return 0;
	}
}

QString HttpUtils::convertResponseStatusCodeNumberToStatusClass(const int& status_code_number)
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

ContentType HttpUtils::detectErrorContentType(const QList<QString> headers)
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

