#include "TestFramework.h"
#include "ServerWrapper.h"
#include "ServerHelper.h"
#include "HttpRequestHandler.h"
#include <QJsonDocument>
#include <QJsonObject>
#include "EndpointManager.h"
#include "ServerController.h"
#include "ServerController.cpp"

TEST_CLASS(WebServer_Test)
{
Q_OBJECT

private slots:

	void test_if_server_is_running()
	{
		if (!ServerHelper::hasBasicSettings())
		{
			SKIP("Server has not been configured correctly");
		}

		QByteArray reply;
		try
		{
			HttpHeaders add_headers;
			add_headers.insert("Accept", "text/html");
			reply = HttpRequestHandler(HttpRequestHandler::NONE).get(ServerHelper::getServerUrl(false) + "/v1/", add_headers);
		}
		catch(Exception& e)
		{
			Log::error(e.message());
			SKIP("This test requieres a running server");
		}
		IS_TRUE(!reply.isEmpty());
	}


	void test_partial_content_multirange_request()
	{
		if (!ServerHelper::hasBasicSettings())
		{
			SKIP("Server has not been configured correctly");
		}

		QByteArray reply;
		try
		{
			HttpHeaders add_headers;
			add_headers.insert("Accept", "text/html");
			add_headers.insert("Range", "bytes=114-140,399-430");
			reply = HttpRequestHandler(HttpRequestHandler::NONE).get(ServerHelper::getServerUrl(false) + "/v1/", add_headers);
		}
		catch(Exception& e)
		{
			Log::error(e.message());
			SKIP("This test requieres a running server");
		}
		IS_TRUE(reply.contains("Welcome to the GSvar server"));
		IS_TRUE(reply.contains("help"));
	}

	void test_partial_content_empty_end_request()
	{
		if (!ServerHelper::hasBasicSettings())
		{
			SKIP("Server has not been configured correctly");
		}

		QByteArray reply;
		try
		{
			HttpHeaders add_headers;
			add_headers.insert("Accept", "text/html");
			add_headers.insert("Range", "bytes=454-");
			reply = HttpRequestHandler(HttpRequestHandler::NONE).get(ServerHelper::getServerUrl(false) + "/v1/", add_headers);
		}
		catch(Exception& e)
		{
			Log::error(e.message());
			SKIP("This test requieres a running server");
		}
		S_EQUAL(reply.trimmed(), "</html>");
	}

	void test_partial_content_empty_start_request()
	{
		if (!ServerHelper::hasBasicSettings())
		{
			SKIP("Server has not been configured correctly");
		}

		QByteArray reply;
		try
		{
			HttpHeaders add_headers;
			add_headers.insert("Accept", "text/html");
			add_headers.insert("Range", "bytes=-8");
			reply = HttpRequestHandler(HttpRequestHandler::NONE).get(ServerHelper::getServerUrl(false) + "/v1/", add_headers);

		}
		catch(Exception& e)
		{
			Log::error(e.message());
			SKIP("This test requieres a running server");
		}
		S_EQUAL(reply.trimmed(), "</html>");

		HttpHeaders add_headers;
		add_headers.insert("Accept", "text/html");
		add_headers.insert("Range", "bytes=0-5,5-8");
		IS_THROWN(Exception, HttpRequestHandler(HttpRequestHandler::NONE).get(ServerHelper::getServerUrl(false) + "/v1/", add_headers));
	}

	void test_basic_http_authentication()
	{
		if (!ServerHelper::hasBasicSettings())
		{
			SKIP("Server has not been configured correctly");
		}

		QByteArray reply;
		try
		{
			HttpHeaders add_headers;
			add_headers.insert("Accept", "text/html");
			reply = HttpRequestHandler(HttpRequestHandler::NONE).get("https://ahmustm1:123456@" + ServerHelper::getStringSettingsValue("server_host") +":" + ServerHelper::getStringSettingsValue("https_server_port") + "/v1/protected", add_headers);
		}
		catch(Exception& e)
		{
			Log::error(e.message());
			SKIP("This test requieres a running server");
		}
		IS_TRUE(reply.contains("Folder content:"));
	}

	void test_access_to_bam_files_over_http()
	{
		if (!ServerHelper::hasBasicSettings())
		{
			SKIP("Server has not been configured correctly");
		}

		QString filename = ServerHelper::getServerUrl(true) + "/v1/bam/rna.bam";

		try
		{
			HttpHeaders add_headers;
			add_headers.insert("Accept", "application/octet-stream");
			QByteArray reply = HttpRequestHandler(HttpRequestHandler::NONE).get(filename, add_headers);
		}
		catch(Exception& e)
		{
			Log::error(e.message());
			SKIP("This test requieres a running server");
		}

		// Read the BAM file, if the server is running
		BamReader reader(filename);
		I_EQUAL(reader.headerLines().count(), 2588);
		I_EQUAL(reader.chromosomes().count(), 2580);
	}

};
