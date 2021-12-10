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

	void test_file_info()
	{
		QString url_id = ServerHelper::generateUniqueStr();
		QByteArray file = TESTDATA("data_in/picture.png");

		IS_FALSE(UrlManager::isInStorageAlready(file));

		UrlManager::addUrlToStorage(url_id, QFileInfo(file).fileName(), QFileInfo(file).absolutePath(), file);
		IS_TRUE(UrlManager::isInStorageAlready(file));

		HttpRequest request;
		request.setMethod(RequestMethod::GET);
		request.setContentType(ContentType::APPLICATION_JSON);
		request.addHeader("host", "localhost:8443");
		request.addHeader("accept", "application/json");
		request.addHeader("connection", "keep-alive");
		request.setPrefix("v1");
		request.setPath("file_info");
		request.addUrlParam("file", "https://localhost:8443/v1/temp/" + url_id + "/picture.png");

		HttpResponse response = EndpointController::getFileInfo(request);

		QJsonDocument document = QJsonDocument::fromJson(response.getPayload());
		QJsonObject json_object = document.object();
		S_EQUAL(json_object.value("file_name").toString(), "picture.png");
		IS_TRUE(json_object.value("exists").toBool());

		QMap<QString, QString> params;
		params.insert("file", "https://localhost:8443/v1/temp/fake_id");
		request.setUrlParams(params);
		response = EndpointController::getFileInfo(request);

		IS_TRUE(response.getStatusLine().split('\n').first().contains("404"));
	}
};
