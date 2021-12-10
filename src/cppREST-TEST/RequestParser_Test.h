#include "TestFramework.h"
#include "RequestParser.h"

TEST_CLASS(RequestParser_Test)
{
Q_OBJECT
private slots:

	void test_request_parser()
	{
		QByteArray raw_request =
			"GET /v1/static/1.png?var=val HTTP/1.1\r\n"
			"Host: localhost:8443\r\n"
			"Connection: keep-alive\r\n";

		RequestParser *parser = new RequestParser();
		HttpRequest parsed_request = parser->parse(&raw_request);

		S_EQUAL(parsed_request.getPath(), "static");
		S_EQUAL(HttpProcessor::convertMethodTypeToString(parsed_request.getMethod()), "get");
		S_EQUAL(parsed_request.getUrlParams().value("var"), "val");
		S_EQUAL(parsed_request.getHeaders()["host"][0], "localhost:8443");
		S_EQUAL(parsed_request.getHeaders()["connection"][0], "keep-alive");

		raw_request.append("Malformed header - value\r\n");
		IS_THROWN(Exception, parser->parse(&raw_request));
	}
};
