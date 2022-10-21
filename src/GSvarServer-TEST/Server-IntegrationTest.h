#ifndef SERVERINTEGRATIONTEST_H
#define SERVERINTEGRATIONTEST_H

#include "TestFramework.h"
#include "ServerWrapper.h"
#include "ServerHelper.h"
#include "HttpRequestHandler.h"
#include <QJsonDocument>
#include <QJsonObject>
#include "EndpointManager.h"
#include "ServerController.h"

int sendGetRequest(QByteArray& reply, QString url, HttpHeaders headers)
{
	try
	{
		reply = HttpRequestHandler(HttpRequestHandler::NONE).get(url, headers);
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
		reply = HttpRequestHandler(HttpRequestHandler::NONE).post(url, data, headers);
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
		if (!ServerHelper::hasBasicSettings())
		{
			SKIP("Server has not been configured correctly");
		}

		QByteArray reply;
		HttpHeaders add_headers;
		add_headers.insert("Accept", "text/html");
		int code = sendGetRequest(reply, ServerHelper::getServerUrl(false) + "/v1/", add_headers);
		if (code > 0)
		{
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
		HttpHeaders add_headers;
		add_headers.insert("Accept", "text/html");
		add_headers.insert("Range", "bytes=114-140,399-430");
		int code = sendGetRequest(reply, ServerHelper::getServerUrl(false) + "/v1/", add_headers);
		if (code > 0)
		{
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
		HttpHeaders add_headers;
		add_headers.insert("Accept", "text/html");
		add_headers.insert("Range", "bytes=454-");
		int code = sendGetRequest(reply, ServerHelper::getServerUrl(false) + "/v1/", add_headers);
		if (code > 0)
		{
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
		HttpHeaders add_headers;
		add_headers.insert("Accept", "text/html");
		add_headers.insert("Range", "bytes=-8");
		int code = sendGetRequest(reply, ServerHelper::getServerUrl(false) + "/v1/", add_headers);
		if (code > 0)
		{
			SKIP("This test requieres a running server");
		}

		S_EQUAL(reply.trimmed(), "</html>");

		add_headers.clear();
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
		HttpHeaders add_headers;
		add_headers.insert("Accept", "text/html");
		int code = sendGetRequest(reply, "https://ahmustm1:123456@" + ServerHelper::getStringSettingsValue("server_host") +":" + ServerHelper::getStringSettingsValue("https_server_port") + "/v1/protected", add_headers);
		if (code > 0)
		{
			SKIP("This test requieres a running server");
		}

		IS_TRUE(reply.contains("Folder content:"));
	}

	void test_token_based_authentication()
	{
		if (!ServerHelper::hasBasicSettings())
		{
			SKIP("Server has not been configured correctly");
		}

		QByteArray reply;
		HttpHeaders add_headers;
		add_headers.insert("Accept", "text/html");
		QByteArray data = "name=ahmustm1&password=123456";
		int code = sendPostRequest(reply, ServerHelper::getServerUrl(true) + "/v1/login", add_headers, data);
		if (code > 0)
		{
			SKIP("This test requieres a running server");
		}
		QString token = reply.trimmed();
		IS_TRUE(!token.isEmpty());

		reply.clear();
		add_headers.clear();
		add_headers.insert("Accept", "application/json");
		code = sendGetRequest(reply, ServerHelper::getServerUrl(true) + "/v1/session?token=" + token, add_headers);
		QJsonDocument session_json = QJsonDocument::fromJson(reply);
		IS_TRUE(session_json.isObject());

		reply.clear();
		add_headers.insert("Authorization", "Bearer "+token.toUtf8());
		code = sendGetRequest(reply, ServerHelper::getServerUrl(true) + "/v1/session", add_headers);
		session_json = QJsonDocument::fromJson(reply);
		IS_TRUE(session_json.isObject());
		bool is_db_token = session_json.object().value("is_db_token").toBool();
		IS_FALSE(is_db_token);
	}

//	void test_access_to_bam_files_over_http()
//	{
//		if (!ServerHelper::hasBasicSettings())
//		{
//			SKIP("Server has not been configured correctly");
//		}

//		QString filename = ServerHelper::getServerUrl(true) + "/v1/bam/rna.bam";

//		QByteArray reply;
//		HttpHeaders add_headers;
//		add_headers.insert("Accept", "application/octet-stream");
//		int code = sendGetRequest(reply, filename, add_headers);
//		if (code > 0)
//		{
//			SKIP("This test requieres a running server");
//		}

//		// Read the BAM file, if the server is running
//		BamReader reader(filename);
//		I_EQUAL(reader.headerLines().count(), 2588);
//		I_EQUAL(reader.chromosomes().count(), 2580);
//	}

	void test_server_info_retrieval()
	{
		if (!ServerHelper::hasBasicSettings())
		{
			SKIP("Server has not been configured correctly");
		}

		QByteArray reply;
		HttpHeaders add_headers;
		add_headers.insert("Accept", "application/json");
		int code = sendGetRequest(reply, ServerHelper::getServerUrl(true) + "/v1/info", add_headers);
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
};

#endif // SERVERINTEGRATIONTEST_H
