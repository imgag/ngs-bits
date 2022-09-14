#include "TestFramework.h"
#include "EndpointController.h"

TEST_CLASS(EndpointController_Test)
{
Q_OBJECT
private slots:

	void test_static_file_random_access()
	{
		QString url_id = ServerHelper::generateUniqueStr();
		QByteArray file = TESTDATA("data_in/text.txt");
		IS_FALSE(UrlManager::isInStorageAlready(file));

		UrlManager::addNewUrl(url_id, UrlEntity(QFileInfo(file).fileName(), QFileInfo(file).absolutePath(), file, url_id, QDateTime::currentDateTime()));
		IS_TRUE(UrlManager::isInStorageAlready(file));

		Session cur_session(1, QDateTime::currentDateTime());
		SessionManager::addNewSession("token", cur_session);

		HttpRequest request;
		request.setMethod(RequestMethod::GET);
		request.setContentType(ContentType::TEXT_HTML);
		request.addHeader("host", "localhost:8443");
		request.addHeader("accept", "text/html");
		request.addHeader("connection", "keep-alive");
		request.addHeader("range", "bytes=10-17");
		request.setPrefix("v1");
		request.setPath("temp");
		request.addPathItem(url_id);
		request.addPathItem("text.txt");
		request.addUrlParam("token", "token");

		HttpResponse response = EndpointController::serveStaticForTempUrl(request);

		IS_TRUE(response.getStatusLine().split('\n').first().contains("206"));
		IS_TRUE(response.getPayload().isNull());

		QList<QString> params;
		params.append("fake_id");
		params.append("text.txt");
		request.setPathItems(params);
		response = EndpointController::serveStaticForTempUrl(request);
		IS_TRUE(response.getStatusLine().split('\n').first().contains("404"));
	}

	void test_head_response_with_empty_body_for_missing_file()
	{
		Session cur_session(1, QDateTime::currentDateTime());
		SessionManager::addNewSession("token", cur_session);

		HttpRequest request;
		request.setMethod(RequestMethod::HEAD);
		request.setContentType(ContentType::TEXT_HTML);
		request.addHeader("host", "localhost:8443");
		request.addHeader("accept", "text/html");
		request.addHeader("connection", "keep-alive");
		request.setPrefix("v1");
		request.setPath("temp");
		request.addPathItem("fake_unique_id");
		request.addPathItem("file.txt");
		request.addUrlParam("token", "token");

		HttpResponse response = EndpointController::serveStaticForTempUrl(request);

		IS_TRUE(response.getStatusLine().split('\n').first().contains("404"));
		IS_TRUE(response.getPayload().isNull());

		QRegExp rx("(length:)(?:\\s*)(\\d+)");
		rx.setCaseSensitivity(Qt::CaseInsensitive);
		int pos = rx.indexIn(response.getHeaders());

		IS_TRUE(pos > -1);
		int length = rx.cap(2).toInt();
		I_EQUAL(length, 0);
	}

	void test_head_response_with_empty_body_for_existing_file()
	{
		QString url_id = ServerHelper::generateUniqueStr();
		QByteArray file = TESTDATA("data_in/text.txt");
		UrlManager::addNewUrl(url_id, UrlEntity(QFileInfo(file).fileName(), QFileInfo(file).absolutePath(), file, url_id, QDateTime::currentDateTime()));

		Session cur_session(1, QDateTime::currentDateTime());
		SessionManager::addNewSession("token", cur_session);

		HttpRequest request;
		request.setMethod(RequestMethod::HEAD);
		request.setContentType(ContentType::TEXT_HTML);
		request.addHeader("host", "localhost:8443");
		request.addHeader("accept", "text/html");
		request.addHeader("connection", "keep-alive");
		request.setPrefix("v1");
		request.setPath("temp");
		request.addPathItem(url_id);
		request.addPathItem("text.txt");
		request.addUrlParam("token", "token");

		HttpResponse response = EndpointController::serveStaticForTempUrl(request);

		IS_TRUE(response.getStatusLine().split('\n').first().contains("200"));
		IS_TRUE(response.getPayload().isNull());

		QRegExp rx("(length:)(?:\\s*)(\\d+)");
		rx.setCaseSensitivity(Qt::CaseInsensitive);
		int pos = rx.indexIn(response.getHeaders());

		IS_TRUE(pos > -1);
		int length = rx.cap(2).toInt();
		I_EQUAL(length, 18);
	}
};
