#include "TestFramework.h"
#include "RequestParser.h"
#include "HttpUtils.h"

TEST_CLASS(RequestParser_Test)
{
private:

	TEST_METHOD(test_parsing_get_request)
	{
		QByteArray raw_request =
			"GET /v1/static/1.png?var=val HTTP/1.1\r\n"
			"Host: localhost:8443\r\n"
			"Connection: keep-alive\r\n";

		RequestParser *parser = new RequestParser();
		HttpRequest parsed_request = parser->parse(&raw_request);

		S_EQUAL(parsed_request.getPath(), "static");
		S_EQUAL(HttpUtils::convertMethodTypeToString(parsed_request.getMethod()), "get");
		S_EQUAL(parsed_request.getUrlParams().value("var"), "val");
		S_EQUAL(parsed_request.getHeaders()["host"][0], "localhost:8443");
		S_EQUAL(parsed_request.getHeaders()["connection"][0], "keep-alive");

		raw_request.append("Malformed header - value\r\n");
		IS_THROWN(Exception, parser->parse(&raw_request));
	}

	TEST_METHOD(test_parsing_multipart_form_post_request)
	{
		QByteArray raw_request =
			"POST /v1/upload?token=token_value HTTP/1.1\r\n"
			"Host: localhost:8443\r\n"
			"Content-Type: multipart/form-data; boundary=------------------------2cb4f6c221043bbe\r\n\r\n"
			"--------------------------2cb4f6c221043bbe\r\n"
			"Content-Disposition: form-data; name=\"form_field\"\r\n\r\n"
			"field_value\r\n"
			"--------------------------2cb4f6c221043bbe\r\n"
			"Content-Disposition: form-data; name=\"another_field\"\r\n\r\n"
			"another_value\r\n"
			"--------------------------2cb4f6c221043bbe\r\n"
			"Content-Disposition: form-data; name=\"file\"; filename=\"README.md\"\r\n"
			"Content-Type: application/octet-stream\r\n\r\n"
			"File content\r\n"
			"--------------------------2cb4f6c221043bbe--";
		RequestParser *parser = new RequestParser();
		HttpRequest parsed_request = parser->parse(&raw_request);
		S_EQUAL(parsed_request.getPath(), "upload");
		S_EQUAL(HttpUtils::convertMethodTypeToString(parsed_request.getMethod()), "post");
		S_EQUAL(parsed_request.getUrlParams().value("token"), "token_value");

		I_EQUAL(parsed_request.getFormDataParams().count(), 2);
		S_EQUAL(parsed_request.getFormDataParams()["form_field"], "field_value");
		S_EQUAL(parsed_request.getFormDataParams()["another_field"], "another_value");
		S_EQUAL(parsed_request.getMultipartFileName(), "README.md");
		S_EQUAL(parsed_request.getMultipartFileContent(), "File content");
	}
};
