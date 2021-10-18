#include "TestFramework.h"
#include "WebServer.h"
#include "ServerHelper.h"
#include "HttpRequestHandler.h"
#include <QJsonDocument>
#include <QJsonObject>
#include "EndpointManager.h"
#include "EndpointHandler.h"
#include "EndpointHandler.cpp"

TEST_CLASS(WebServer_Test)
{
Q_OBJECT

private slots:

	void test_if_server_is_running()
	{
		QByteArray reply;
		try
		{
			HttpHeaders add_headers;
			add_headers.insert("Accept", "text/html");
			reply = HttpRequestHandler(HttpRequestHandler::NONE).get("https://localhost:8443/v1/", add_headers);
		}
		catch(Exception& e)
		{
			qDebug() << e.message();
			SKIP("This test requieres a running server");
		}
		IS_TRUE(!reply.isEmpty());

		QThread::sleep(5);
		try
		{
			qDebug() << "Sending a request";
			HttpHeaders add_headers;
			add_headers.insert("Accept", "application/json");
			reply = HttpRequestHandler(HttpRequestHandler::NONE).get("https://localhost:8443/v1/fakepage", add_headers);
		}
		catch(Exception& e)
		{
			reply = e.message().toUtf8();

		}
		IS_TRUE(reply.indexOf("This action cannot be processed")!=-1);
	}


	void test_partial_content_multirange_request()
	{
		QByteArray reply;
		try
		{
			HttpHeaders add_headers;
			add_headers.insert("Accept", "text/html");
			add_headers.insert("Range", "bytes=251-282,1369-1374");
			reply = HttpRequestHandler(HttpRequestHandler::NONE).get("https://localhost:8443/v1/", add_headers);
		}
		catch(Exception& e)
		{
			qDebug() << e.message();
			SKIP("This test requieres a running server");
		}
		IS_TRUE(reply.contains("Welcome to GSvarServer info page"));
		IS_TRUE(reply.contains("looks"));
	}

	void test_partial_content_empty_end_request()
	{
		QByteArray reply;
		try
		{
			HttpHeaders add_headers;
			add_headers.insert("Accept", "text/html");
			add_headers.insert("Range", "bytes=1830-");
			reply = HttpRequestHandler(HttpRequestHandler::NONE).get("https://localhost:8443/v1/", add_headers);
		}
		catch(Exception& e)
		{
			qDebug() << e.message();
			SKIP("This test requieres a running server");
		}
		S_EQUAL(reply.trimmed(), "</html>");

	}

	void test_partial_content_empty_start_request()
	{
		QByteArray reply;
		try
		{
			HttpHeaders add_headers;
			add_headers.insert("Accept", "text/html");
			add_headers.insert("Range", "bytes=-8");
			reply = HttpRequestHandler(HttpRequestHandler::NONE).get("https://localhost:8443/v1/", add_headers);
		}
		catch(Exception& e)
		{
			qDebug() << e.message();
			SKIP("This test requieres a running server");
		}
		S_EQUAL(reply.trimmed(), "</html>");

	}

	void test_partial_content_overlapping_ranges_request()
	{
		HttpHeaders add_headers;
		add_headers.insert("Accept", "text/html");
		add_headers.insert("Range", "bytes=0-5,5-8");
		IS_THROWN(Exception, HttpRequestHandler(HttpRequestHandler::NONE).get("https://localhost:8443/v1/", add_headers));
	}

	void test_basic_http_authentication()
	{
		QByteArray reply;
		try
		{
			HttpHeaders add_headers;
			add_headers.insert("Accept", "text/html");
			reply = HttpRequestHandler(HttpRequestHandler::NONE).get("https://ahmustm1:123456@localhost:8443/v1/protected", add_headers);
		}
		catch(Exception& e)
		{
			SKIP("This test requieres a running server");
		}

		IS_TRUE(reply.contains("Folder content:"))
	}
};
