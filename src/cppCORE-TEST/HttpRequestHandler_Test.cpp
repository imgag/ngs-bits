#include "TestFramework.h"
#include "HttpRequestHandler.h"
#include "Settings.h"

TEST_CLASS(HttpRequestHandler_Test)
{
private:
	TEST_METHOD(get_with_proxy)
	{
		//at UKT, we need proxy credentials...
		QString url = "https://raw.githubusercontent.com/imgag/ngs-bits/refs/heads/master/src/cppCORE-TEST/data_in/txt_file.txt";
		if (HttpRequestHandler::proxyForUrl(url).endsWith(".med.uni-tuebingen.de") && Settings::string("proxy_password").trimmed().isEmpty()) return;

		HttpRequestHandler handler;
		ServerReply reply = handler.get(url);
		I_EQUAL(reply.status_code, 200);
		S_EQUAL(reply.body, "##comment\n#header\nthis is a plain text file")
	}

	//TODO Alexandr add tests for all methods
};