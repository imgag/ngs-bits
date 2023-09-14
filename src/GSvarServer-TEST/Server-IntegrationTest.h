#ifndef SERVERINTEGRATIONTEST_H
#define SERVERINTEGRATIONTEST_H

#include "TestFramework.h"
#include "ServerWrapper.h"
#include "ServerHelper.h"
#include "HttpRequestHandler.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkProxy>
#include "EndpointManager.h"
#include "ServerController.h"

int sendGetRequest(QByteArray& reply, QString url, HttpHeaders headers)
{
	try
	{
        reply = HttpRequestHandler(QNetworkProxy(QNetworkProxy::NoProxy)).get(url, headers).body;
	}
	catch(Exception& e)
	{
		if(e.message().contains("Network error 1", Qt::CaseInsensitive))
		{
			return 1;
		}
		Log::error(e.message());
	}
	return 0;
}

int sendPostRequest(QByteArray& reply, QString url, HttpHeaders headers, QByteArray data)
{
	try
	{
        reply = HttpRequestHandler(QNetworkProxy(QNetworkProxy::NoProxy)).post(url, data, headers).body;
	}
	catch(Exception& e)
	{
		if(e.message().contains("Network error 1", Qt::CaseInsensitive))
		{
			return 1;
		}
		Log::error(e.message());
	}
	return 0;
}

TEST_CLASS(Server_IntegrationTest)
{
Q_OBJECT

private slots:

	void test_if_server_is_running()
	{
        if (!ServerHelper::hasMinimalSettings())
		{
			SKIP("Server has not been configured correctly");
		}

		QByteArray reply;
		HttpHeaders add_headers;
		add_headers.insert("Accept", "text/html");
        add_headers.insert("Content-Type", "text/html");
		int code = sendGetRequest(reply, ClientHelper::serverApiUrl(), add_headers);
		if (code > 0)
		{
			SKIP("This test requieres a running server");
		}
		IS_TRUE(!reply.isEmpty());
	}


	void test_partial_content_multirange_request()
	{
        if (!ServerHelper::hasMinimalSettings())
		{
			SKIP("Server has not been configured correctly");
		}

		QByteArray reply;
		HttpHeaders add_headers;
		add_headers.insert("Accept", "text/html");
        add_headers.insert("Content-Type", "text/html");
		add_headers.insert("Range", "bytes=114-140,399-430");
		int code = sendGetRequest(reply, ClientHelper::serverApiUrl(), add_headers);
		if (code > 0)
		{
			SKIP("This test requieres a running server");
		}

		IS_TRUE(reply.contains("Welcome to the GSvar server"));
		IS_TRUE(reply.contains("help"));
	}

	void test_partial_content_empty_end_request()
	{
        if (!ServerHelper::hasMinimalSettings())
		{
			SKIP("Server has not been configured correctly");
		}

		QByteArray reply;
		HttpHeaders add_headers;
		add_headers.insert("Accept", "text/html");
        add_headers.insert("Content-Type", "text/html");
		add_headers.insert("Range", "bytes=454-");
		int code = sendGetRequest(reply, ClientHelper::serverApiUrl(), add_headers);
		if (code > 0)
		{
			SKIP("This test requieres a running server");
		}

		S_EQUAL(reply.trimmed(), "</html>");
	}

	void test_partial_content_empty_start_request()
	{
        if (!ServerHelper::hasMinimalSettings())
		{
			SKIP("Server has not been configured correctly");
		}

		QByteArray reply;
		HttpHeaders add_headers;
		add_headers.insert("Accept", "text/html");
        add_headers.insert("Content-Type", "text/html");
		add_headers.insert("Range", "bytes=-8");
		int code = sendGetRequest(reply, ClientHelper::serverApiUrl(), add_headers);
		if (code > 0)
		{
			SKIP("This test requieres a running server");
		}

		S_EQUAL(reply.trimmed(), "</html>");

		add_headers.clear();
		add_headers.insert("Accept", "text/html");
        add_headers.insert("Content-Type", "text/html");
		add_headers.insert("Range", "bytes=0-5,5-8");
		IS_THROWN(Exception, HttpRequestHandler(QNetworkProxy(QNetworkProxy::NoProxy)).get(ClientHelper::serverApiUrl(), add_headers));
	}	

	void test_token_based_authentication()
	{
        if (!ServerHelper::hasMinimalSettings())
		{
			SKIP("Server has not been configured correctly");
		}

		QByteArray reply;
		HttpHeaders add_headers;
		add_headers.insert("Accept", "text/html");
        add_headers.insert("Content-Type", "application/x-www-form-urlencoded");
        QByteArray data = "name=ahmustm1&password=123456";
		int code = sendPostRequest(reply, ClientHelper::serverApiUrl() + "login", add_headers, data);
		if (code > 0)
		{
			SKIP("This test requieres a running server");
		}
		QString token = reply.trimmed();
		IS_TRUE(!token.isEmpty());

		reply.clear();
		add_headers.clear();
		add_headers.insert("Accept", "application/json");
        add_headers.insert("Content-Type", "application/json");
		code = sendGetRequest(reply, ClientHelper::serverApiUrl() + "session?token=" + token, add_headers);
		QJsonDocument session_json = QJsonDocument::fromJson(reply);
		IS_TRUE(session_json.isObject());

		reply.clear();
		add_headers.insert("Authorization", "Bearer "+token.toUtf8());
		code = sendGetRequest(reply, ClientHelper::serverApiUrl() + "session", add_headers);
		session_json = QJsonDocument::fromJson(reply);
		IS_TRUE(session_json.isObject());
		bool is_db_token = session_json.object().value("is_db_token").toBool();
		IS_FALSE(is_db_token);
	}

	void test_access_to_remote_bam_files()
	{
        if (!ServerHelper::hasMinimalSettings())
		{
			SKIP("Server has not been configured correctly");
		}

		QString filename = ClientHelper::serverApiUrl() + "bam/rna.bam";

		QByteArray reply;
		HttpHeaders add_headers;
		add_headers.insert("Accept", "application/octet-stream");
        add_headers.insert("Content-Type", "application/octet-stream");
		int code = sendGetRequest(reply, filename, add_headers);
		if (code > 0)
		{
			SKIP("This test requieres a running server");
		}

		// Read the BAM file, if the server is running
		BamReader reader(filename);
		I_EQUAL(reader.headerLines().count(), 2588);
		I_EQUAL(reader.chromosomes().count(), 2580);
	}

	void test_server_info_retrieval()
	{
        if (!ServerHelper::hasMinimalSettings())
		{
			SKIP("Server has not been configured correctly");
		}

		QByteArray reply;
		HttpHeaders add_headers;
		add_headers.insert("Accept", "application/json");
        add_headers.insert("Content-Type", "application/json");
		int code = sendGetRequest(reply, ClientHelper::serverApiUrl() + "info", add_headers);
		if (code > 0)
		{
			SKIP("This test requieres a running server");
		}

		QJsonDocument doc = QJsonDocument::fromJson(reply);
		IS_TRUE(doc.isObject());
		IS_TRUE(doc.object().contains("version"));
		IS_TRUE(doc.object().contains("api_version"));
		S_EQUAL(doc.object()["api_version"].toString(), "v1");
		IS_TRUE(doc.object().contains("start_time"));
	}

	void test_client_info_retrieval()
	{
        if (!ServerHelper::hasMinimalSettings())
		{
			SKIP("Server has not been configured correctly");
		}

		QByteArray reply;
		HttpHeaders add_headers;
		add_headers.insert("Accept", "application/json");
        add_headers.insert("Content-Type", "application/json");
		int code = sendGetRequest(reply, ClientHelper::serverApiUrl() + "current_client", add_headers);
		if (code > 0)
		{
			SKIP("This test requieres a running server");
		}

		QJsonDocument out = QJsonDocument::fromJson(reply);
		IS_TRUE(out.isObject());
	}
};

#endif // SERVERINTEGRATIONTEST_H
