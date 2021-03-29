#include "TestFramework.h"
#include "HttpProcessor.h"
#include "HttpProcessor.cpp"

TEST_CLASS(HttpProcessor_Test)
{
Q_OBJECT
private slots:
	void test_getContentTypeFromString()
	{
		ContentType type = HttpProcessor::getContentTypeFromString("application/octet-stream");
		S_EQUAL(type, ContentType::APPLICATION_OCTET_STREAM);

		type = HttpProcessor::getContentTypeFromString("application/json");
		S_EQUAL(type, ContentType::APPLICATION_JSON);

		type = HttpProcessor::getContentTypeFromString("application/javascript");
		S_EQUAL(type, ContentType::APPLICATION_JAVASCRIPT);

		type = HttpProcessor::getContentTypeFromString("image/jpeg");
		S_EQUAL(type, ContentType::IMAGE_JPEG);

		type = HttpProcessor::getContentTypeFromString("image/png");
		S_EQUAL(type, ContentType::IMAGE_PNG);

		type = HttpProcessor::getContentTypeFromString("image/svg+xml");
		S_EQUAL(type, ContentType::IMAGE_SVG_XML);

		type = HttpProcessor::getContentTypeFromString("text/plain");
		S_EQUAL(type, ContentType::TEXT_PLAIN);

		type = HttpProcessor::getContentTypeFromString("text/csv");
		S_EQUAL(type, ContentType::TEXT_CSV);

		type = HttpProcessor::getContentTypeFromString("text/html");
		S_EQUAL(type, ContentType::TEXT_HTML);

		type = HttpProcessor::getContentTypeFromString("text/xml");
		S_EQUAL(type, ContentType::TEXT_XML);

		type = HttpProcessor::getContentTypeFromString("text/css");
		S_EQUAL(type, ContentType::TEXT_CSS);

		type = HttpProcessor::getContentTypeFromString("multipart/form-data");
		S_EQUAL(type, ContentType::MULTIPART_FORM_DATA);
	}

	void test_getMethodTypeFromString()
	{
		RequestMethod type = HttpProcessor::getMethodTypeFromString("get");
		S_EQUAL(type, RequestMethod::GET);

		type = HttpProcessor::getMethodTypeFromString("post");
		S_EQUAL(type, RequestMethod::POST);

		type = HttpProcessor::getMethodTypeFromString("delete");
		S_EQUAL(type, RequestMethod::DELETE);

		type = HttpProcessor::getMethodTypeFromString("put");
		S_EQUAL(type, RequestMethod::PUT);

		type = HttpProcessor::getMethodTypeFromString("patch");
		S_EQUAL(type, RequestMethod::PATCH);
	}

	void test_convertMethodTypeToString()
	{
		QString type = HttpProcessor::convertMethodTypeToString(RequestMethod::GET);
		S_EQUAL(type, "get");

		type = HttpProcessor::convertMethodTypeToString(RequestMethod::POST);
		S_EQUAL(type, "post");

		type = HttpProcessor::convertMethodTypeToString(RequestMethod::DELETE);
		S_EQUAL(type, "delete");

		type = HttpProcessor::convertMethodTypeToString(RequestMethod::PUT);
		S_EQUAL(type, "put");

		type = HttpProcessor::convertMethodTypeToString(RequestMethod::PATCH);
		S_EQUAL(type, "patch");
	}

	void test_convertContentTypeToString()
	{
		QString type = HttpProcessor::convertContentTypeToString(ContentType::APPLICATION_OCTET_STREAM);
		S_EQUAL(type, "application/octet-stream");

		type = HttpProcessor::convertContentTypeToString(ContentType::APPLICATION_JSON);
		S_EQUAL(type, "application/json");

		type = HttpProcessor::convertContentTypeToString(ContentType::APPLICATION_JAVASCRIPT);
		S_EQUAL(type, "application/javascript");

		type = HttpProcessor::convertContentTypeToString(ContentType::IMAGE_JPEG);
		S_EQUAL(type, "image/jpeg");

		type = HttpProcessor::convertContentTypeToString(ContentType::IMAGE_PNG);
		S_EQUAL(type, "image/png");

		type = HttpProcessor::convertContentTypeToString(ContentType::IMAGE_SVG_XML);
		S_EQUAL(type, "image/svg+xml");

		type = HttpProcessor::convertContentTypeToString(ContentType::TEXT_PLAIN);
		S_EQUAL(type, "text/plain");

		type = HttpProcessor::convertContentTypeToString(ContentType::TEXT_CSV);
		S_EQUAL(type, "text/csv");

		type = HttpProcessor::convertContentTypeToString(ContentType::TEXT_HTML);
		S_EQUAL(type, "text/html");

		type = HttpProcessor::convertContentTypeToString(ContentType::TEXT_XML);
		S_EQUAL(type, "text/xml");

		type = HttpProcessor::convertContentTypeToString(ContentType::TEXT_CSS);
		S_EQUAL(type, "text/css");

		type = HttpProcessor::convertContentTypeToString(ContentType::MULTIPART_FORM_DATA);
		S_EQUAL(type, "multipart/form-data");
	}

	void test_getContentTypeByFilename()
	{
		ContentType type = HttpProcessor::getContentTypeByFilename("json");
		S_EQUAL(type, ContentType::APPLICATION_JSON);

		type = HttpProcessor::getContentTypeByFilename("js");
		S_EQUAL(type, ContentType::APPLICATION_JAVASCRIPT);

		type = HttpProcessor::getContentTypeByFilename("jpeg");
		S_EQUAL(type, ContentType::IMAGE_JPEG);

		type = HttpProcessor::getContentTypeByFilename("jpg");
		S_EQUAL(type, ContentType::IMAGE_JPEG);

		type = HttpProcessor::getContentTypeByFilename("png");
		S_EQUAL(type, ContentType::IMAGE_PNG);

		type = HttpProcessor::getContentTypeByFilename("svg");
		S_EQUAL(type, ContentType::IMAGE_SVG_XML);

		type = HttpProcessor::getContentTypeByFilename("txt");
		S_EQUAL(type, ContentType::TEXT_PLAIN);

		type = HttpProcessor::getContentTypeByFilename("csv");
		S_EQUAL(type, ContentType::TEXT_CSV);

		type = HttpProcessor::getContentTypeByFilename("html");
		S_EQUAL(type, ContentType::TEXT_HTML);

		type = HttpProcessor::getContentTypeByFilename("htm");
		S_EQUAL(type, ContentType::TEXT_HTML);

		type = HttpProcessor::getContentTypeByFilename("xml");
		S_EQUAL(type, ContentType::TEXT_XML);

		type = HttpProcessor::getContentTypeByFilename("css");
		S_EQUAL(type, ContentType::TEXT_CSS);
	}

	void test_convertErrorTypeToText()
	{
		QString error_msg = HttpProcessor::convertStatusCodeToReasonPhrase(StatusCode::BAD_REQUEST);
		S_EQUAL(error_msg, "Bad Request");

		error_msg = HttpProcessor::convertStatusCodeToReasonPhrase(StatusCode::UNAUTHORIZED);
		S_EQUAL(error_msg, "Unauthorized");

		error_msg = HttpProcessor::convertStatusCodeToReasonPhrase(StatusCode::PAYMENT_REQUIRED);
		S_EQUAL(error_msg, "Payment Required");

		error_msg = HttpProcessor::convertStatusCodeToReasonPhrase(StatusCode::FORBIDDEN);
		S_EQUAL(error_msg, "Forbidden");

		error_msg = HttpProcessor::convertStatusCodeToReasonPhrase(StatusCode::NOT_FOUND);
		S_EQUAL(error_msg, "Not Found");

		error_msg = HttpProcessor::convertStatusCodeToReasonPhrase(StatusCode::METHOD_NOT_ALLOWED);
		S_EQUAL(error_msg, "Method Not Allowed");

		error_msg = HttpProcessor::convertStatusCodeToReasonPhrase(StatusCode::NOT_ACCEPTABLE);
		S_EQUAL(error_msg, "Not Acceptable");

		error_msg = HttpProcessor::convertStatusCodeToReasonPhrase(StatusCode::PROXY_AUTH_REQUIRED);
		S_EQUAL(error_msg, "Proxy Authentication Required");

		error_msg = HttpProcessor::convertStatusCodeToReasonPhrase(StatusCode::REQUEST_TIMEOUT);
		S_EQUAL(error_msg, "Request Timeout");

		error_msg = HttpProcessor::convertStatusCodeToReasonPhrase(StatusCode::CONFLICT);
		S_EQUAL(error_msg, "Conflict");

		error_msg = HttpProcessor::convertStatusCodeToReasonPhrase(StatusCode::GONE);
		S_EQUAL(error_msg, "Gone");

		error_msg = HttpProcessor::convertStatusCodeToReasonPhrase(StatusCode::LENGTH_REQUIRED);
		S_EQUAL(error_msg, "Length Required");

		error_msg = HttpProcessor::convertStatusCodeToReasonPhrase(StatusCode::PRECONDITION_FAILED);
		S_EQUAL(error_msg, "Precondition Failed");

		error_msg = HttpProcessor::convertStatusCodeToReasonPhrase(StatusCode::ENTITY_TOO_LARGE);
		S_EQUAL(error_msg, "Request Entity Too Large");

		error_msg = HttpProcessor::convertStatusCodeToReasonPhrase(StatusCode::URI_TOO_LONG);
		S_EQUAL(error_msg, "Request-URI Too Long");

		error_msg = HttpProcessor::convertStatusCodeToReasonPhrase(StatusCode::UNSUPPORTED_MEDIA_TYPE);
		S_EQUAL(error_msg, "Unsupported Media Type");

		error_msg = HttpProcessor::convertStatusCodeToReasonPhrase(StatusCode::RANGE_NOT_SATISFIABLE);
		S_EQUAL(error_msg, "Requested Range Not Satisfiable");

		error_msg = HttpProcessor::convertStatusCodeToReasonPhrase(StatusCode::EXPECTATION_FAILED);
		S_EQUAL(error_msg, "Expectation Failed");

		error_msg = HttpProcessor::convertStatusCodeToReasonPhrase(StatusCode::INTERNAL_SERVER_ERROR);
		S_EQUAL(error_msg, "Internal Server Error");

		error_msg = HttpProcessor::convertStatusCodeToReasonPhrase(StatusCode::NOT_IMPLEMENTED);
		S_EQUAL(error_msg, "Not Implemented");

		error_msg = HttpProcessor::convertStatusCodeToReasonPhrase(StatusCode::BAD_GATEWAY);
		S_EQUAL(error_msg, "Bad Gateway");

		error_msg = HttpProcessor::convertStatusCodeToReasonPhrase(StatusCode::SERVICE_UNAVAILABLE);
		S_EQUAL(error_msg, "Service Unavailable");

		error_msg = HttpProcessor::convertStatusCodeToReasonPhrase(StatusCode::GATEWAY_TIMEOUT);
		S_EQUAL(error_msg, "Gateway Timeout");

		error_msg = HttpProcessor::convertStatusCodeToReasonPhrase(StatusCode::HTTP_VERSION_NOT_SUPPORTED);
		S_EQUAL(error_msg, "HTTP Version Not Supported");

		error_msg = HttpProcessor::convertStatusCodeToReasonPhrase(StatusCode::UNKNOWN_STATUS_CODE);
		S_EQUAL(error_msg, "Unknown Status Code");
	}

	void test_getErrorCodeByType()
	{
		int code = HttpProcessor::convertStatusCodeToInt(StatusCode::BAD_REQUEST);
		I_EQUAL(code, 400);

		code = HttpProcessor::convertStatusCodeToInt(StatusCode::UNAUTHORIZED);
		I_EQUAL(code, 401);

		code = HttpProcessor::convertStatusCodeToInt(StatusCode::PAYMENT_REQUIRED);
		I_EQUAL(code, 402);

		code = HttpProcessor::convertStatusCodeToInt(StatusCode::FORBIDDEN);
		I_EQUAL(code, 403);

		code = HttpProcessor::convertStatusCodeToInt(StatusCode::NOT_FOUND);
		I_EQUAL(code, 404);

		code = HttpProcessor::convertStatusCodeToInt(StatusCode::METHOD_NOT_ALLOWED);
		I_EQUAL(code, 405);

		code = HttpProcessor::convertStatusCodeToInt(StatusCode::NOT_ACCEPTABLE);
		I_EQUAL(code, 406);

		code = HttpProcessor::convertStatusCodeToInt(StatusCode::PROXY_AUTH_REQUIRED);
		I_EQUAL(code, 407);

		code = HttpProcessor::convertStatusCodeToInt(StatusCode::REQUEST_TIMEOUT);
		I_EQUAL(code, 408);

		code = HttpProcessor::convertStatusCodeToInt(StatusCode::CONFLICT);
		I_EQUAL(code, 409);

		code = HttpProcessor::convertStatusCodeToInt(StatusCode::GONE);
		I_EQUAL(code, 410);

		code = HttpProcessor::convertStatusCodeToInt(StatusCode::LENGTH_REQUIRED);
		I_EQUAL(code, 411);

		code = HttpProcessor::convertStatusCodeToInt(StatusCode::PRECONDITION_FAILED);
		I_EQUAL(code, 412);

		code = HttpProcessor::convertStatusCodeToInt(StatusCode::ENTITY_TOO_LARGE);
		I_EQUAL(code, 413);

		code = HttpProcessor::convertStatusCodeToInt(StatusCode::URI_TOO_LONG);
		I_EQUAL(code, 414);

		code = HttpProcessor::convertStatusCodeToInt(StatusCode::UNSUPPORTED_MEDIA_TYPE);
		I_EQUAL(code, 415);

		code = HttpProcessor::convertStatusCodeToInt(StatusCode::RANGE_NOT_SATISFIABLE);
		I_EQUAL(code, 416);

		code = HttpProcessor::convertStatusCodeToInt(StatusCode::EXPECTATION_FAILED);
		I_EQUAL(code, 417);

		code = HttpProcessor::convertStatusCodeToInt(StatusCode::INTERNAL_SERVER_ERROR);
		I_EQUAL(code, 500);

		code = HttpProcessor::convertStatusCodeToInt(StatusCode::NOT_IMPLEMENTED);
		I_EQUAL(code, 501);

		code = HttpProcessor::convertStatusCodeToInt(StatusCode::BAD_GATEWAY);
		I_EQUAL(code, 502);

		code = HttpProcessor::convertStatusCodeToInt(StatusCode::SERVICE_UNAVAILABLE);
		I_EQUAL(code, 503);

		code = HttpProcessor::convertStatusCodeToInt(StatusCode::GATEWAY_TIMEOUT);
		I_EQUAL(code, 504);

		code = HttpProcessor::convertStatusCodeToInt(StatusCode::HTTP_VERSION_NOT_SUPPORTED);
		I_EQUAL(code, 505);

		code = HttpProcessor::convertStatusCodeToInt(StatusCode::UNKNOWN_STATUS_CODE);
		I_EQUAL(code, 0);
	}
};
