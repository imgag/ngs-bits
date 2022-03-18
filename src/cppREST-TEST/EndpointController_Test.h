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

		UrlManager::addUrlToStorage(url_id, QFileInfo(file).fileName(), QFileInfo(file).absolutePath(), file);
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
		request.addPathParam(url_id);
		request.addPathParam("text.txt");
		request.addUrlParam("token", "token");

		HttpResponse response = EndpointController::serveStaticForTempUrl(request);

		IS_TRUE(response.getStatusLine().split('\n').first().contains("206"));
		IS_TRUE(response.getPayload().isNull());

		QList<QString> params;
		params.append("fake_id");
		params.append("text.txt");
		request.setPathParams(params);
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
		request.addPathParam("fake_unique_id");
		request.addPathParam("file.txt");
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
		UrlManager::addUrlToStorage(url_id, QFileInfo(file).fileName(), QFileInfo(file).absolutePath(), file);

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
		request.addPathParam(url_id);
		request.addPathParam("text.txt");
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
