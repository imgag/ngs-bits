#include "TestFramework.h"
#include "HttpUtils.h"

TEST_CLASS(HttpProcessor_Test)
{
Q_OBJECT
private slots:
	void test_getContentTypeFromString()
	{
		ContentType type = HttpUtils::getContentTypeFromString("application/octet-stream");
		S_EQUAL(type, ContentType::APPLICATION_OCTET_STREAM);

		type = HttpUtils::getContentTypeFromString("application/json");
		S_EQUAL(type, ContentType::APPLICATION_JSON);

		type = HttpUtils::getContentTypeFromString("application/javascript");
		S_EQUAL(type, ContentType::APPLICATION_JAVASCRIPT);

		type = HttpUtils::getContentTypeFromString("image/jpeg");
		S_EQUAL(type, ContentType::IMAGE_JPEG);

		type = HttpUtils::getContentTypeFromString("image/png");
		S_EQUAL(type, ContentType::IMAGE_PNG);

		type = HttpUtils::getContentTypeFromString("image/svg+xml");
		S_EQUAL(type, ContentType::IMAGE_SVG_XML);

		type = HttpUtils::getContentTypeFromString("text/plain");
		S_EQUAL(type, ContentType::TEXT_PLAIN);

		type = HttpUtils::getContentTypeFromString("text/csv");
		S_EQUAL(type, ContentType::TEXT_CSV);

		type = HttpUtils::getContentTypeFromString("text/html");
		S_EQUAL(type, ContentType::TEXT_HTML);

		type = HttpUtils::getContentTypeFromString("text/xml");
		S_EQUAL(type, ContentType::TEXT_XML);

		type = HttpUtils::getContentTypeFromString("text/css");
		S_EQUAL(type, ContentType::TEXT_CSS);

		type = HttpUtils::getContentTypeFromString("multipart/form-data");
		S_EQUAL(type, ContentType::MULTIPART_FORM_DATA);
	}

	void test_getMethodTypeFromString()
	{
		RequestMethod type = HttpUtils::getMethodTypeFromString("get");
		S_EQUAL(type, RequestMethod::GET);

		type = HttpUtils::getMethodTypeFromString("post");
		S_EQUAL(type, RequestMethod::POST);

		type = HttpUtils::getMethodTypeFromString("delete");
		S_EQUAL(type, RequestMethod::DELETE);

		type = HttpUtils::getMethodTypeFromString("put");
		S_EQUAL(type, RequestMethod::PUT);

		type = HttpUtils::getMethodTypeFromString("patch");
		S_EQUAL(type, RequestMethod::PATCH);
	}

	void test_convertMethodTypeToString()
	{
		QString type = HttpUtils::convertMethodTypeToString(RequestMethod::GET);
		S_EQUAL(type, "get");

		type = HttpUtils::convertMethodTypeToString(RequestMethod::POST);
		S_EQUAL(type, "post");

		type = HttpUtils::convertMethodTypeToString(RequestMethod::DELETE);
		S_EQUAL(type, "delete");

		type = HttpUtils::convertMethodTypeToString(RequestMethod::PUT);
		S_EQUAL(type, "put");

		type = HttpUtils::convertMethodTypeToString(RequestMethod::PATCH);
		S_EQUAL(type, "patch");
	}

	void test_convertContentTypeToString()
	{
		QString type = HttpUtils::convertContentTypeToString(ContentType::APPLICATION_OCTET_STREAM);
		S_EQUAL(type, "application/octet-stream");

		type = HttpUtils::convertContentTypeToString(ContentType::APPLICATION_JSON);
		S_EQUAL(type, "application/json");

		type = HttpUtils::convertContentTypeToString(ContentType::APPLICATION_JAVASCRIPT);
		S_EQUAL(type, "application/javascript");

		type = HttpUtils::convertContentTypeToString(ContentType::IMAGE_JPEG);
		S_EQUAL(type, "image/jpeg");

		type = HttpUtils::convertContentTypeToString(ContentType::IMAGE_PNG);
		S_EQUAL(type, "image/png");

		type = HttpUtils::convertContentTypeToString(ContentType::IMAGE_SVG_XML);
		S_EQUAL(type, "image/svg+xml");

		type = HttpUtils::convertContentTypeToString(ContentType::TEXT_PLAIN);
		S_EQUAL(type, "text/plain");

		type = HttpUtils::convertContentTypeToString(ContentType::TEXT_CSV);
		S_EQUAL(type, "text/csv");

		type = HttpUtils::convertContentTypeToString(ContentType::TEXT_HTML);
		S_EQUAL(type, "text/html");

		type = HttpUtils::convertContentTypeToString(ContentType::TEXT_XML);
		S_EQUAL(type, "text/xml");

		type = HttpUtils::convertContentTypeToString(ContentType::TEXT_CSS);
		S_EQUAL(type, "text/css");

		type = HttpUtils::convertContentTypeToString(ContentType::MULTIPART_FORM_DATA);
		S_EQUAL(type, "multipart/form-data");
	}

	void test_getContentTypeByFilename()
	{
		ContentType type = HttpUtils::getContentTypeByFilename("json");
		S_EQUAL(type, ContentType::APPLICATION_JSON);

		type = HttpUtils::getContentTypeByFilename("js");
		S_EQUAL(type, ContentType::APPLICATION_JAVASCRIPT);

		type = HttpUtils::getContentTypeByFilename("jpeg");
		S_EQUAL(type, ContentType::IMAGE_JPEG);

		type = HttpUtils::getContentTypeByFilename("jpg");
		S_EQUAL(type, ContentType::IMAGE_JPEG);

		type = HttpUtils::getContentTypeByFilename("png");
		S_EQUAL(type, ContentType::IMAGE_PNG);

		type = HttpUtils::getContentTypeByFilename("svg");
		S_EQUAL(type, ContentType::IMAGE_SVG_XML);

		type = HttpUtils::getContentTypeByFilename("txt");
		S_EQUAL(type, ContentType::TEXT_PLAIN);

		type = HttpUtils::getContentTypeByFilename("csv");
		S_EQUAL(type, ContentType::TEXT_CSV);

		type = HttpUtils::getContentTypeByFilename("html");
		S_EQUAL(type, ContentType::TEXT_HTML);

		type = HttpUtils::getContentTypeByFilename("htm");
		S_EQUAL(type, ContentType::TEXT_HTML);

		type = HttpUtils::getContentTypeByFilename("xml");
		S_EQUAL(type, ContentType::TEXT_XML);

		type = HttpUtils::getContentTypeByFilename("css");
		S_EQUAL(type, ContentType::TEXT_CSS);
	}

	void test_convertErrorTypeToText()
	{
		QString error_msg = HttpUtils::convertResponseStatusToReasonPhrase(ResponseStatus::BAD_REQUEST);
		S_EQUAL(error_msg, "Bad Request");

		error_msg = HttpUtils::convertResponseStatusToReasonPhrase(ResponseStatus::UNAUTHORIZED);
		S_EQUAL(error_msg, "Unauthorized");

		error_msg = HttpUtils::convertResponseStatusToReasonPhrase(ResponseStatus::PAYMENT_REQUIRED);
		S_EQUAL(error_msg, "Payment Required");

		error_msg = HttpUtils::convertResponseStatusToReasonPhrase(ResponseStatus::FORBIDDEN);
		S_EQUAL(error_msg, "Forbidden");

		error_msg = HttpUtils::convertResponseStatusToReasonPhrase(ResponseStatus::NOT_FOUND);
		S_EQUAL(error_msg, "Not Found");

		error_msg = HttpUtils::convertResponseStatusToReasonPhrase(ResponseStatus::METHOD_NOT_ALLOWED);
		S_EQUAL(error_msg, "Method Not Allowed");

		error_msg = HttpUtils::convertResponseStatusToReasonPhrase(ResponseStatus::NOT_ACCEPTABLE);
		S_EQUAL(error_msg, "Not Acceptable");

		error_msg = HttpUtils::convertResponseStatusToReasonPhrase(ResponseStatus::PROXY_AUTH_REQUIRED);
		S_EQUAL(error_msg, "Proxy Authentication Required");

		error_msg = HttpUtils::convertResponseStatusToReasonPhrase(ResponseStatus::REQUEST_TIMEOUT);
		S_EQUAL(error_msg, "Request Timeout");

		error_msg = HttpUtils::convertResponseStatusToReasonPhrase(ResponseStatus::CONFLICT);
		S_EQUAL(error_msg, "Conflict");

		error_msg = HttpUtils::convertResponseStatusToReasonPhrase(ResponseStatus::GONE);
		S_EQUAL(error_msg, "Gone");

		error_msg = HttpUtils::convertResponseStatusToReasonPhrase(ResponseStatus::LENGTH_REQUIRED);
		S_EQUAL(error_msg, "Length Required");

		error_msg = HttpUtils::convertResponseStatusToReasonPhrase(ResponseStatus::PRECONDITION_FAILED);
		S_EQUAL(error_msg, "Precondition Failed");

		error_msg = HttpUtils::convertResponseStatusToReasonPhrase(ResponseStatus::ENTITY_TOO_LARGE);
		S_EQUAL(error_msg, "Request Entity Too Large");

		error_msg = HttpUtils::convertResponseStatusToReasonPhrase(ResponseStatus::URI_TOO_LONG);
		S_EQUAL(error_msg, "Request-URI Too Long");

		error_msg = HttpUtils::convertResponseStatusToReasonPhrase(ResponseStatus::UNSUPPORTED_MEDIA_TYPE);
		S_EQUAL(error_msg, "Unsupported Media Type");

		error_msg = HttpUtils::convertResponseStatusToReasonPhrase(ResponseStatus::RANGE_NOT_SATISFIABLE);
		S_EQUAL(error_msg, "Requested Range Not Satisfiable");

		error_msg = HttpUtils::convertResponseStatusToReasonPhrase(ResponseStatus::EXPECTATION_FAILED);
		S_EQUAL(error_msg, "Expectation Failed");

		error_msg = HttpUtils::convertResponseStatusToReasonPhrase(ResponseStatus::INTERNAL_SERVER_ERROR);
		S_EQUAL(error_msg, "Internal Server Error");

		error_msg = HttpUtils::convertResponseStatusToReasonPhrase(ResponseStatus::NOT_IMPLEMENTED);
		S_EQUAL(error_msg, "Not Implemented");

		error_msg = HttpUtils::convertResponseStatusToReasonPhrase(ResponseStatus::BAD_GATEWAY);
		S_EQUAL(error_msg, "Bad Gateway");

		error_msg = HttpUtils::convertResponseStatusToReasonPhrase(ResponseStatus::SERVICE_UNAVAILABLE);
		S_EQUAL(error_msg, "Service Unavailable");

		error_msg = HttpUtils::convertResponseStatusToReasonPhrase(ResponseStatus::GATEWAY_TIMEOUT);
		S_EQUAL(error_msg, "Gateway Timeout");

		error_msg = HttpUtils::convertResponseStatusToReasonPhrase(ResponseStatus::HTTP_VERSION_NOT_SUPPORTED);
		S_EQUAL(error_msg, "HTTP Version Not Supported");

		error_msg = HttpUtils::convertResponseStatusToReasonPhrase(ResponseStatus::UNKNOWN_STATUS_CODE);
		S_EQUAL(error_msg, "Unknown Status Code");
	}

	void test_getErrorCodeByType()
	{
		int code = HttpUtils::convertResponseStatusToStatusCodeNumber(ResponseStatus::BAD_REQUEST);
		I_EQUAL(code, 400);

		code = HttpUtils::convertResponseStatusToStatusCodeNumber(ResponseStatus::UNAUTHORIZED);
		I_EQUAL(code, 401);

		code = HttpUtils::convertResponseStatusToStatusCodeNumber(ResponseStatus::PAYMENT_REQUIRED);
		I_EQUAL(code, 402);

		code = HttpUtils::convertResponseStatusToStatusCodeNumber(ResponseStatus::FORBIDDEN);
		I_EQUAL(code, 403);

		code = HttpUtils::convertResponseStatusToStatusCodeNumber(ResponseStatus::NOT_FOUND);
		I_EQUAL(code, 404);

		code = HttpUtils::convertResponseStatusToStatusCodeNumber(ResponseStatus::METHOD_NOT_ALLOWED);
		I_EQUAL(code, 405);

		code = HttpUtils::convertResponseStatusToStatusCodeNumber(ResponseStatus::NOT_ACCEPTABLE);
		I_EQUAL(code, 406);

		code = HttpUtils::convertResponseStatusToStatusCodeNumber(ResponseStatus::PROXY_AUTH_REQUIRED);
		I_EQUAL(code, 407);

		code = HttpUtils::convertResponseStatusToStatusCodeNumber(ResponseStatus::REQUEST_TIMEOUT);
		I_EQUAL(code, 408);

		code = HttpUtils::convertResponseStatusToStatusCodeNumber(ResponseStatus::CONFLICT);
		I_EQUAL(code, 409);

		code = HttpUtils::convertResponseStatusToStatusCodeNumber(ResponseStatus::GONE);
		I_EQUAL(code, 410);

		code = HttpUtils::convertResponseStatusToStatusCodeNumber(ResponseStatus::LENGTH_REQUIRED);
		I_EQUAL(code, 411);

		code = HttpUtils::convertResponseStatusToStatusCodeNumber(ResponseStatus::PRECONDITION_FAILED);
		I_EQUAL(code, 412);

		code = HttpUtils::convertResponseStatusToStatusCodeNumber(ResponseStatus::ENTITY_TOO_LARGE);
		I_EQUAL(code, 413);

		code = HttpUtils::convertResponseStatusToStatusCodeNumber(ResponseStatus::URI_TOO_LONG);
		I_EQUAL(code, 414);

		code = HttpUtils::convertResponseStatusToStatusCodeNumber(ResponseStatus::UNSUPPORTED_MEDIA_TYPE);
		I_EQUAL(code, 415);

		code = HttpUtils::convertResponseStatusToStatusCodeNumber(ResponseStatus::RANGE_NOT_SATISFIABLE);
		I_EQUAL(code, 416);

		code = HttpUtils::convertResponseStatusToStatusCodeNumber(ResponseStatus::EXPECTATION_FAILED);
		I_EQUAL(code, 417);

		code = HttpUtils::convertResponseStatusToStatusCodeNumber(ResponseStatus::INTERNAL_SERVER_ERROR);
		I_EQUAL(code, 500);

		code = HttpUtils::convertResponseStatusToStatusCodeNumber(ResponseStatus::NOT_IMPLEMENTED);
		I_EQUAL(code, 501);

		code = HttpUtils::convertResponseStatusToStatusCodeNumber(ResponseStatus::BAD_GATEWAY);
		I_EQUAL(code, 502);

		code = HttpUtils::convertResponseStatusToStatusCodeNumber(ResponseStatus::SERVICE_UNAVAILABLE);
		I_EQUAL(code, 503);

		code = HttpUtils::convertResponseStatusToStatusCodeNumber(ResponseStatus::GATEWAY_TIMEOUT);
		I_EQUAL(code, 504);

		code = HttpUtils::convertResponseStatusToStatusCodeNumber(ResponseStatus::HTTP_VERSION_NOT_SUPPORTED);
		I_EQUAL(code, 505);

		code = HttpUtils::convertResponseStatusToStatusCodeNumber(ResponseStatus::UNKNOWN_STATUS_CODE);
		I_EQUAL(code, 0);
	}
};
