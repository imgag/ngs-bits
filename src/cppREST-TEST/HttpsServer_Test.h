#include "TestFramework.h"
#include "HttpsServer.h"
#include "ServerHelper.h"
#include "HttpRequestHandler.h"
#include <QJsonDocument>
#include <QJsonObject>
#include "EndpointManager.h"
#include "EndpointHandler.h"
#include "EndpointHandler.cpp"

TEST_CLASS(HttpsServer_Test)
{
Q_OBJECT

private slots:

	void test_if_server_is_running()
	{
		QByteArray reply;
		try
		{
			HttpHeaders add_headers;
			add_headers.insert("Accept", "application/json");
			reply = HttpRequestHandler(HttpRequestHandler::NONE).get("https://localhost:8443/v1/", add_headers);
		}
		catch(Exception& e)
		{
			qDebug() << "Error while getting API info:" << e.message();
			SKIP("The server is probably not available");
		}

		QJsonDocument json_doc = QJsonDocument::fromJson(reply);
		QJsonObject json_obj = json_doc.object();

		qDebug() << reply;
		IS_TRUE(!reply.isEmpty());
		S_EQUAL(json_obj.value("name").toString(), "GSvarServer");

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
};
