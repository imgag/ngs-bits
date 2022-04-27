#include "TestFramework.h"
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

		HttpResponse response = EndpointHandler::serveResourceAsset(request);
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


		Session cur_session(1, QDateTime::currentDateTime());
		SessionManager::addNewSession("token", cur_session);

		HttpRequest request;
		request.setMethod(RequestMethod::PUT);
		request.setContentType(ContentType::TEXT_HTML);
		request.setPrefix("v1");
		request.setPath("project_file");
		request.addUrlParam("ps_url_id", url_id);
		request.setBody(json_doc.toJson());
		request.addUrlParam("token", "token");

		HttpResponse response = EndpointHandler::saveProjectFile(request);
		IS_TRUE(response.getStatusLine().contains("200"));
		COMPARE_FILES(file_copy, TESTDATA("data/sample_saved_changes.gsvar"));
		QFile::remove(copy_name);
	}

	void test_uploading_file()
	{
		QString url_id = ServerHelper::generateUniqueStr();
		QString file = TESTDATA("data/sample.gsvar");
		QString copy_name = "uploaded_file.txt";
		QByteArray upload_file = TESTDATA("data/to_upload.txt");

		IS_FALSE(UrlManager::isInStorageAlready(upload_file));
		UrlManager::addUrlToStorage(url_id, QFileInfo(upload_file).fileName(), QFileInfo(upload_file).absolutePath(), upload_file);
		IS_TRUE(UrlManager::isInStorageAlready(upload_file));

		Session cur_session(1, QDateTime::currentDateTime());
		SessionManager::addNewSession("token", cur_session);

		HttpRequest request;
		request.setMethod(RequestMethod::POST);
		request.setContentType(ContentType::MULTIPART_FORM_DATA);
		request.setPrefix("v1");
		request.setPath("upload");
		request.addUrlParam("token", "token");

		request.setMultipartFileName(copy_name);
		request.setMultipartFileContent(Helper::loadTextFile(upload_file)[0].toLocal8Bit());

		request.addHeader("Accept", "*/*");
		request.addHeader("Content-Type", "multipart/form-data; boundary=------------------------2cb4f6c221043bbe");

		HttpResponse response = EndpointHandler::uploadFile(request);
		IS_TRUE(response.getStatusLine().contains("400"));
		request.addFormDataParam("ps_url_id", url_id);
		response = EndpointHandler::uploadFile(request);
		IS_TRUE(response.getStatusLine().contains("200"));
		QString file_copy = TESTDATA("data/" + copy_name.toLocal8Bit());
		COMPARE_FILES(file_copy, upload_file);
		QFile::remove(file_copy);
	}
};
