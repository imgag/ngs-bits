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
			"Connection: keep-alive\r\n"
			"\r\n";

		RequestParser *parser = new RequestParser();
		HttpRequest parsed_request = parser->parse(&raw_request);

		S_EQUAL(parsed_request.getPath(), "static");
		S_EQUAL(HttpUtils::convertMethodTypeToString(parsed_request.getMethod()), "get");
		S_EQUAL(parsed_request.getUrlParams().value("var"), "val");
		S_EQUAL(parsed_request.getHeaders()["host"][0], "localhost:8443");
		S_EQUAL(parsed_request.getHeaders()["connection"][0], "keep-alive");

		raw_request.prepend("Malformed header - value\r\n");
		IS_THROWN(Exception, parser->parse(&raw_request));
	}

	TEST_METHOD(test_parsing_multipart_form_text_post_request)
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

	TEST_METHOD(test_parsing_multipart_form_binary_post_request)
	{
		QFile png_file(TESTDATA("data_in/picture.png"));
		I_EQUAL(png_file.size(), 41236);
		IS_TRUE(png_file.open(QFile::ReadOnly));
		QByteArray png_file_content = png_file.readAll();
		I_EQUAL(png_file_content.size(), 41236);

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
			"Content-Disposition: form-data; name=\"file\"; filename=\"picture.png\"\r\n"
			"Content-Type: image/png\r\n";
		raw_request += "\r\n";
		raw_request += png_file_content;
		raw_request += "\r\n";
		raw_request += "--------------------------2cb4f6c221043bbe--";

		RequestParser *parser = new RequestParser();
		HttpRequest parsed_request = parser->parse(&raw_request);
		S_EQUAL(parsed_request.getPath(), "upload");
		S_EQUAL(HttpUtils::convertMethodTypeToString(parsed_request.getMethod()), "post");
		S_EQUAL(parsed_request.getUrlParams().value("token"), "token_value");
		I_EQUAL(parsed_request.getFormDataParams().count(), 2);
		S_EQUAL(parsed_request.getMultipartFileName(), "picture.png");
		QByteArray uploaded_content = parsed_request.getMultipartFileContent();
		I_EQUAL(uploaded_content.size(), png_file.size());

		// valid PNG images have a fixed starting and ending sequences
		const QByteArray png_start_signature = QByteArray::fromHex("89504E470D0A1A0A");
		const QByteArray png_end_signature = QByteArray::fromHex("0000000049454E44AE426082");
		IS_TRUE(uploaded_content.startsWith(png_start_signature));
		IS_TRUE(uploaded_content.endsWith(png_end_signature));
	}
};
