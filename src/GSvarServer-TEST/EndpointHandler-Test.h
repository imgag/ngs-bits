#include "TestFramework.h"
#include "SessionManager.h"
#include "EndpointHandler.h"
#include "EndpointHandler.cpp"

TEST_CLASS(EndpointHandler_Test)
{
Q_OBJECT
private slots:
	void test_api_info()
    {
		HttpRequest request;
		request.setMethod(RequestMethod::GET);
		request.setContentType(ContentType::APPLICATION_JSON);
		request.setPrefix("v1");
		request.setPath("info");

		HttpResponse response = EndpointHandler::serveApiInfo(request);
		IS_TRUE(response.getStatusLine().contains("200"));
		IS_TRUE(response.getFilename().contains("api.json"));
    }

	void test_saving_gsvar_file()
	{
		QString url_id = ServerHelper::generateUniqueStr();
		QString file = TESTDATA("data/sample.gsvar");
		QString copy_name = file+"_tmp";
		QFile::copy(file, copy_name);
		QString file_copy = TESTDATA(copy_name.toLocal8Bit());

		IS_FALSE(UrlManager::isInStorageAlready(file_copy));
		UrlManager::addUrlToStorage(url_id, QFileInfo(file_copy).fileName(), QFileInfo(file_copy).absolutePath(), file_copy);
		IS_TRUE(UrlManager::isInStorageAlready(file_copy));

		QJsonDocument json_doc = QJsonDocument();
		QJsonArray json_array;
		QJsonObject json_object;
		json_object.insert("variant", "chr1:12062205-12062205 A>G");
		json_object.insert("column", "comment");
		json_object.insert("text", "some text for testing");
		json_array.append(json_object);
		json_doc.setArray(json_array);

		HttpRequest request;
		request.setMethod(RequestMethod::PUT);
		request.setContentType(ContentType::TEXT_HTML);
		request.setPrefix("v1");
		request.setPath("project_file");
		request.addUrlParam("ps_url_id", url_id);
		request.setBody(json_doc.toJson());

		HttpResponse response = EndpointHandler::saveProjectFile(request);
		IS_TRUE(response.getStatusLine().contains("200"));

		QFile::remove(copy_name);
	}
};
