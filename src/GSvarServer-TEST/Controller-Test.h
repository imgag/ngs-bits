#include "TestFramework.h"
#include "ServerController.h"
#include "ServerController.cpp"

TEST_CLASS(Controller_Test)
{
Q_OBJECT
private slots:
	void test_api_info()
    {
		if (!ServerHelper::settingsValid(true))
        {
            SKIP("Server has not been configured correctly");
        }

        ServerDB().reinitializeDb();
        HttpRequest request;
		request.setMethod(RequestMethod::GET);
		request.setContentType(ContentType::APPLICATION_JSON);
		request.setPrefix("v1");
		request.setPath("info");

		HttpResponse response = ServerController::serveResourceAsset(request);
		QJsonDocument json_doc = QJsonDocument::fromJson(response.getPayload());	;

		IS_TRUE(response.getStatusLine().contains("200"));
		S_EQUAL(json_doc.object()["name"].toString(), ToolBase::applicationName());
		S_EQUAL(json_doc.object()["version"].toString(), ToolBase::version());
		S_EQUAL(json_doc.object()["api_version"].toString(), ClientHelper::serverApiVersion());
    }

	void test_saving_gsvar_file()
	{
		if (!ServerHelper::settingsValid(true))
        {
            SKIP("Server has not been configured correctly");
        }

        QString url_id = ServerHelper::generateUniqueStr();
		QString file = TESTDATA("data/sample.gsvar");
		QString copy_name = file+"_tmp";
		QFile::copy(file, copy_name);
		QString file_copy = TESTDATA(copy_name.toUtf8());

		IS_FALSE(UrlManager::isInStorageAlready(file_copy));
        QFileInfo info = QFileInfo(file_copy);
        UrlManager::addNewUrl(UrlEntity(url_id, info.fileName(), info.absolutePath(), file_copy, url_id, info.size(), info.exists(), QDateTime::currentDateTime()));
		IS_TRUE(UrlManager::isInStorageAlready(file_copy));

		QJsonDocument json_doc = QJsonDocument();
		QJsonArray json_array;
		QJsonObject json_object;
		json_object.insert("variant", "chr1:12062205-12062205 A>G");
		json_object.insert("column", "comment");
		json_object.insert("text", "some text for testing");
		json_array.append(json_object);
		json_doc.setArray(json_array);


        Session cur_session("gsvar_token", 1, "jsmith", "John Smith", QDateTime::currentDateTime());
        SessionManager::addNewSession(cur_session);

		HttpRequest request;
		request.setMethod(RequestMethod::PUT);
		request.setContentType(ContentType::TEXT_HTML);
		request.setPrefix("v1");
		request.setPath("project_file");
		request.addUrlParam("ps_url_id", url_id);
		request.setBody(json_doc.toJson());
        request.addUrlParam("token", "gsvar_token");

		HttpResponse response = ServerController::saveProjectFile(request);
		IS_TRUE(response.getStatusLine().contains("200"));
		COMPARE_FILES(file_copy, TESTDATA("data/sample_saved_changes.gsvar"));
		QFile::remove(copy_name);
	}

	void test_uploading_file()
	{
		if (!ServerHelper::settingsValid(true))
        {
            SKIP("Server has not been configured correctly");
        }

        QString url_id = ServerHelper::generateUniqueStr();
		QString file = TESTDATA("data/sample.gsvar");
		QString copy_name = "uploaded_file.txt";
		QByteArray upload_file = TESTDATA("data/to_upload.txt");

		IS_FALSE(UrlManager::isInStorageAlready(upload_file));
        QFileInfo info = QFileInfo(upload_file);
        UrlManager::addNewUrl(UrlEntity(url_id, info.fileName(), info.absolutePath(), upload_file, url_id, info.size(), info.exists(), QDateTime::currentDateTime()));
		IS_TRUE(UrlManager::isInStorageAlready(upload_file));

        Session cur_session("upload_token", 1, "jsmith", "John Smith", QDateTime::currentDateTime());
        SessionManager::addNewSession(cur_session);

		HttpRequest request;
		request.setMethod(RequestMethod::POST);
		request.setContentType(ContentType::MULTIPART_FORM_DATA);
		request.setPrefix("v1");
		request.setPath("upload");
        request.addUrlParam("token", "upload_token");

		request.setMultipartFileName(copy_name);
		request.setMultipartFileContent(Helper::loadTextFile(upload_file)[0].toUtf8());

		request.addHeader("Accept", "*/*");
		request.addHeader("Content-Type", "multipart/form-data; boundary=------------------------2cb4f6c221043bbe");

		HttpResponse response = ServerController::uploadFile(request);
		IS_TRUE(response.getStatusLine().contains("400"));
        request.addUrlParam("ps_url_id", url_id);
		response = ServerController::uploadFile(request);
		IS_TRUE(response.getStatusLine().contains("200"));
		QString file_copy = TESTDATA("data/" + copy_name.toUtf8());
		COMPARE_FILES(file_copy, upload_file);
		QFile::remove(file_copy);
	}

	void test_session_info()
	{
		if (!ServerHelper::settingsValid(true))
        {
            SKIP("Server has not been configured correctly");
        }

        QDateTime login_time = QDateTime::currentDateTime();
        qint64 login_time_as_num = login_time.toSecsSinceEpoch();
        Session cur_session("test_session_info_token", 1, "jsmith", "John Smith", login_time, 0);
        SessionManager::addNewSession(cur_session);

		HttpRequest request;
		request.setMethod(RequestMethod::GET);
		request.setContentType(ContentType::APPLICATION_JSON);
		request.setPrefix("v1");
		request.setPath("session");

		HttpResponse response = ServerController::getSessionInfo(request);
		I_EQUAL(response.getStatusCode(), 403);

        request.addUrlParam("token", "test_session_info_token");
		response = ServerController::getSessionInfo(request);
		QJsonDocument json_doc = QJsonDocument::fromJson(response.getPayload());
		QJsonObject  json_object = json_doc.object();


		I_EQUAL(response.getStatusCode(), 200);
		I_EQUAL(json_object.value("user_id").toInt(), 1);
        I_EQUAL(json_object.value("login_time").toInt(), login_time_as_num);
		IS_FALSE(json_object.value("is_db_token").toBool());
	}

	void test_static_file_random_access()
	{
		if (!ServerHelper::settingsValid(true))
        {
            SKIP("Server has not been configured correctly");
        }

        QString url_id = ServerHelper::generateUniqueStr();
		QByteArray file = TESTDATA("data/text.txt");
		IS_FALSE(UrlManager::isInStorageAlready(file));
        QFileInfo info = QFileInfo(file);
        UrlManager::addNewUrl(UrlEntity(url_id, info.fileName(), info.absolutePath(), file, url_id, info.size(), info.exists(), QDateTime::currentDateTime()));
		IS_TRUE(UrlManager::isInStorageAlready(file));

        Session cur_session("static_file_token", 1, "jsmith", "John Smith", QDateTime::currentDateTime(), 0);
        SessionManager::addNewSession(cur_session);

		HttpRequest request;
		request.setMethod(RequestMethod::GET);
		request.setContentType(ContentType::TEXT_HTML);
		request.addHeader("host", "localhost:8443");
		request.addHeader("accept", "text/html");
        request.addHeader("content-type", "text/html");
		request.addHeader("connection", "keep-alive");
		request.addHeader("range", "bytes=10-17");
		request.setPrefix("v1");
		request.setPath("temp");
		request.addPathItem(url_id);
		request.addPathItem("text.txt");
        request.addUrlParam("token", "static_file_token");

		HttpResponse response = ServerController::serveStaticFromTempUrl(request);

		IS_TRUE(response.getStatusLine().split('\n').first().contains("206"));
		IS_TRUE(response.getPayload().isNull());

		QList<QString> params;
		params.append("fake_id");
		params.append("text.txt");
		request.setPathItems(params);
		response = ServerController::serveStaticFromTempUrl(request);
		IS_TRUE(response.getStatusLine().split('\n').first().contains("404"));
	}

	void test_head_response_with_empty_body_for_missing_file()
	{
		if (!ServerHelper::settingsValid(true))
        {
            SKIP("Server has not been configured correctly");
        }

        Session cur_session("head_response_empty_token", 1, "jsmith", "John Smith", QDateTime::currentDateTime());
        SessionManager::addNewSession(cur_session);

		HttpRequest request;
		request.setMethod(RequestMethod::HEAD);
		request.setContentType(ContentType::TEXT_HTML);
		request.addHeader("host", "localhost:8443");
		request.addHeader("accept", "text/html");
        request.addHeader("content-type", "text/html");
		request.addHeader("connection", "keep-alive");
		request.setPrefix("v1");
		request.setPath("temp");
		request.addPathItem("fake_unique_id");
		request.addPathItem("file.txt");
        request.addUrlParam("token", "head_response_empty_token");

		HttpResponse response = ServerController::serveStaticFromTempUrl(request);

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
		if (!ServerHelper::settingsValid(true))
        {
            SKIP("Server has not been configured correctly");
        }

        QString url_id = ServerHelper::generateUniqueStr();
		QByteArray file = TESTDATA("data/text.txt");
        QFileInfo info = QFileInfo(file);
        UrlManager::addNewUrl(UrlEntity(url_id, info.fileName(), info.absolutePath(), file, url_id, info.size(), info.exists(), QDateTime::currentDateTime()));

        Session cur_session("head_response_exists_token", 1, "jsmith", "John Smith", QDateTime::currentDateTime());
        SessionManager::addNewSession(cur_session);

		HttpRequest request;
		request.setMethod(RequestMethod::HEAD);
		request.setContentType(ContentType::TEXT_HTML);
		request.addHeader("host", "localhost:8443");
		request.addHeader("accept", "text/html");
        request.addHeader("content-type", "text/html");
		request.addHeader("connection", "keep-alive");
		request.setPrefix("v1");
		request.setPath("temp");
		request.addPathItem(url_id);
		request.addPathItem("text.txt");
        request.addUrlParam("token", "head_response_exists_token");

		HttpResponse response = ServerController::serveStaticFromTempUrl(request);

		IS_TRUE(response.getStatusLine().split('\n').first().contains("200"));
		IS_TRUE(response.getPayload().isNull());

		QRegExp rx("(length:)(?:\\s*)(\\d+)");
		rx.setCaseSensitivity(Qt::CaseInsensitive);
		int pos = rx.indexIn(response.getHeaders());

		IS_TRUE(pos > -1);
		int length = rx.cap(2).toInt();
		I_EQUAL(length, 18);
	}

	void test_current_client_info()
	{
		if (!ServerHelper::settingsValid(true))
        {
            SKIP("Server has not been configured correctly");
        }

        ClientInfo current_info("2023_02-21", "New updates available!");
		SessionManager::setCurrentClientInfo(current_info);

		HttpRequest request;
		request.setMethod(RequestMethod::HEAD);
		request.setContentType(ContentType::TEXT_HTML);
		request.addHeader("host", "localhost:8443");
		request.addHeader("accept", "text/html");
        request.addHeader("content-type", "text/html");
		request.addHeader("connection", "keep-alive");
		request.setPrefix("v1");
		request.setPath("current_client");

		HttpResponse response = ServerController::getCurrentClientInfo(request);

		IS_TRUE(response.getStatusLine().split('\n').first().contains("200"));
		IS_FALSE(response.getPayload().isNull());

		QJsonDocument out = QJsonDocument::fromJson(response.getPayload());
		IS_TRUE(out.isObject());
		S_EQUAL(out.object().value("version").toString(), current_info.version);
		S_EQUAL(out.object().value("message").toString(), current_info.message);
	}

	void test_user_notification()
	{
		if (!ServerHelper::settingsValid(true))
        {
            SKIP("Server has not been configured correctly");
        }

        QString notification_message = "Server will be updated!";
		SessionManager::setCurrentNotification(notification_message);

		HttpRequest request;
		request.setMethod(RequestMethod::HEAD);
		request.setContentType(ContentType::TEXT_HTML);
		request.addHeader("host", "localhost:8443");
		request.addHeader("accept", "text/html");
        request.addHeader("content-type", "text/html");
		request.addHeader("connection", "keep-alive");
		request.setPrefix("v1");
		request.setPath("notification");

		HttpResponse response = ServerController::getCurrentNotification(request);

		IS_TRUE(response.getStatusLine().split('\n').first().contains("200"));
		IS_FALSE(response.getPayload().isNull());

		QJsonDocument out = QJsonDocument::fromJson(response.getPayload());
		IS_TRUE(out.isObject());
		IS_TRUE(!out.object().value("id").toString().isEmpty());
		S_EQUAL(out.object().value("message").toString(), notification_message);
	}

	void test_file_upload()
	{
		if (!ServerHelper::settingsValid(true))
        {
            SKIP("Server has not been configured correctly");
        }

        QString test_filename = "test_file.txt";
		QByteArray test_content = "content";

        HttpRequest request;
        request.setMethod(RequestMethod::HEAD);
        request.setContentType(ContentType::TEXT_HTML);
        request.addHeader("host", "localhost:8443");
        request.addHeader("accept", "text/html");
        request.addHeader("content-type", "text/html");
        request.addHeader("connection", "keep-alive");
        request.setPrefix("v1");
        request.setPath("upload");
        request.setMultipartFileName(test_filename);
        request.setMultipartFileContent(test_content);

        HttpResponse upload_response = ServerController::uploadFileToFolder(QDir::tempPath(), request);
		IS_TRUE(upload_response.getStatus() == ResponseStatus::OK);
		S_EQUAL(QFileInfo(upload_response.getPayload()).fileName(), test_filename);

		QSharedPointer<QFile> outfile = Helper::openFileForReading(upload_response.getPayload());
		S_EQUAL(outfile.data()->readAll(), test_content);
	}

    void test_locate_file_by_type()
    {
        if (!ServerHelper::settingsValid(true))
        {
            SKIP("Server has not been configured correctly");
        }

        QString url_id = ServerHelper::generateUniqueStr();
        QString file = TESTDATA("data/sample.gsvar");

        QFileInfo info = QFileInfo(file);
        UrlManager::addNewUrl(UrlEntity(url_id, info.fileName(), info.absolutePath(), file, url_id, info.size(), info.exists(), QDateTime::currentDateTime()));

        Session cur_session("gsvar_token", 1, "jsmith", "John Smith", QDateTime::currentDateTime());
        SessionManager::addNewSession(cur_session);

        HttpRequest request;

        QMap<QString, QString> url_params;
        url_params.insert("ps_url_id", url_id);
        url_params.insert("type", "GSVAR1");
        url_params.insert("path", "");
        url_params.insert("locus", "");
        url_params.insert("multiple_files", 0);
        url_params.insert("return_if_missing", 0);
        url_params.insert("token", "gsvar_token");

        request.setMethod(RequestMethod::GET);
        request.setContentType(ContentType::APPLICATION_JSON);
        request.setPrefix("v1");
        request.setPath("file_location");
        request.setUrlParams(url_params);
        IS_THROWN(Exception, ServerController::locateFileByType(request));

        url_params.insert("type", "GSVAR");
        request.setUrlParams(url_params);
        HttpResponse response = ServerController::locateFileByType(request);
        IS_TRUE(response.getStatusLine().contains("200"));

        QJsonDocument json_result = QJsonDocument::fromJson(response.getPayload());
        IS_TRUE(json_result.isArray());

        // This test is intended to be changed when PathType changes, OTHER is always the last element,
        // it will always change when items are added or deleted
        I_EQUAL(static_cast<int>(PathType::OTHER), 45);
    }
};
