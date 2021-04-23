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

		RequestPaser *parser = new RequestPaser(&raw_request, "127.0.0.1");
		HttpRequest parsed_request = parser->getRequest();

		S_EQUAL(parsed_request.getPath(), "static");
		S_EQUAL(parsed_request.getRemoteAddress(), "127.0.0.1");
		S_EQUAL(HttpProcessor::convertMethodTypeToString(parsed_request.getMethod()), "get");
		S_EQUAL(parsed_request.getUrlParams().value("var"), "val");
		S_EQUAL(parsed_request.getHeaders().value("host"), "localhost:8443");
		S_EQUAL(parsed_request.getHeaders().value("connection"), "keep-alive");

		raw_request.append("Malformed header - value\r\n");
		IS_THROWN(Exception, parser->getRequest());
	}
};
