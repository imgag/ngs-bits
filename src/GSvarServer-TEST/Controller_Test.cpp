#include "TestFramework.h"
#include "ServerController.h"
#include "ServerController.cpp"
#include "ToolBase.h"

TEST_CLASS(Controller_Test)
{
private:

	TEST_METHOD(test_api_info)
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
		S_EQUAL(json_doc.object()["name"].toString(), ServerHelper::getAppName());
		S_EQUAL(json_doc.object()["version"].toString(), ToolBase::version());
		S_EQUAL(json_doc.object()["api_version"].toString(), ClientHelper::serverApiVersion());
    }

	TEST_METHOD(test_saving_gsvar_file)
	{
		if (!ServerHelper::settingsValid(true))
        {
            SKIP("Server has not been configured correctly");
        }

        QString url_id = ServerHelper::generateUniqueStr();
		QString file = TESTDATA("data/sample.gsvar");
		QString copy_name = file+"_tmp";
		QFile::copy(file, copy_name);
        QString file_copy = TESTDATA("data/sample.gsvar_tmp");

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

		Session cur_session("gsvar_token", 1, "jsmith", "John Smith", Helper::randomString(128), QDateTime::currentDateTime());
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

	TEST_METHOD(test_uploading_file)
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

		Session cur_session("upload_token", 1, "jsmith", "John Smith", Helper::randomString(128), QDateTime::currentDateTime());
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

	TEST_METHOD(test_session)
	{
		if (!ServerHelper::settingsValid(true))
        {
            SKIP("Server has not been configured correctly");
        }

        QDateTime login_time = QDateTime::currentDateTime();
        qint64 login_time_as_num = login_time.toSecsSinceEpoch();
		QString random_secret = Helper::randomString(128);
		Session cur_session("test_session_info_token", 1, "jsmith", "John Smith", random_secret,login_time, 0);
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
		IS_FALSE(json_object.contains("random_secret"));

		request.setPath("secret");
		response = ServerController::getRandomSecret(request);

		I_EQUAL(response.getStatusCode(), 200);
		S_EQUAL(response.getPayload(), random_secret);
	}

	TEST_METHOD(test_static_file_random_access)
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

		Session cur_session("static_file_token", 1, "jsmith", "John Smith", Helper::randomString(128), QDateTime::currentDateTime(), 0);
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

	TEST_METHOD(test_head_response_with_empty_body_for_missing_file)
	{
		if (!ServerHelper::settingsValid(true))
        {
            SKIP("Server has not been configured correctly");
        }

		Session cur_session("head_response_empty_token", 1, "jsmith", "John Smith", Helper::randomString(128), QDateTime::currentDateTime());
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

        QRegularExpression rx("(length:)(?:\\s*)(\\d+)", QRegularExpression::CaseInsensitiveOption);
        QRegularExpressionMatch match = rx.match(response.getHeaders());

        int pos = match.hasMatch() ? match.capturedStart(2) : -1;
		IS_TRUE(pos > -1);

        int length = match.hasMatch() ? match.captured(2).toInt() : 0;
        I_EQUAL(length, 0);
	}

	TEST_METHOD(test_head_response_with_empty_body_for_existing_file)
	{
		if (!ServerHelper::settingsValid(true))
        {
            SKIP("Server has not been configured correctly");
        }

        QString url_id = ServerHelper::generateUniqueStr();
		QByteArray file = TESTDATA("data/text.txt");
        QFileInfo info = QFileInfo(file);
        UrlManager::addNewUrl(UrlEntity(url_id, info.fileName(), info.absolutePath(), file, url_id, info.size(), info.exists(), QDateTime::currentDateTime()));

		Session cur_session("head_response_exists_token", 1, "jsmith", "John Smith", Helper::randomString(128), QDateTime::currentDateTime());
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

        QRegularExpression rx("(length:)(?:\\s*)(\\d+)", QRegularExpression::CaseInsensitiveOption);
        QRegularExpressionMatch match = rx.match(response.getHeaders());

        int pos = match.hasMatch() ? match.capturedStart(2) : -1;
		IS_TRUE(pos > -1);

        int length = match.hasMatch() ? match.captured(2).toInt() : 0;
        I_EQUAL(length, 18);
	}

	TEST_METHOD(test_current_client_info)
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

	TEST_METHOD(test_user_notification)
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

	TEST_METHOD(test_file_upload)
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

	TEST_METHOD(test_locate_file_by_type)
    {
        if (!ServerHelper::settingsValid(true))
        {
            SKIP("Server has not been configured correctly");
        }

        QString url_id = ServerHelper::generateUniqueStr();
        QString file = TESTDATA("data/sample.gsvar");

        QFileInfo info = QFileInfo(file);
        UrlManager::addNewUrl(UrlEntity(url_id, info.fileName(), info.absolutePath(), file, url_id, info.size(), info.exists(), QDateTime::currentDateTime()));

		Session cur_session("gsvar_token", 1, "jsmith", "John Smith", Helper::randomString(128), QDateTime::currentDateTime());
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
		I_EQUAL(static_cast<int>(PathType::OTHER), 49);
    }

	TEST_METHOD(test_data_import_api)
	{
		if (!NGSD::isAvailable(true)) SKIP("No test database found!");
		NGSD test_db(true);
		test_db.init();
		test_db.executeQueriesFromFile(TESTDATA("data/NGSD_in5.sql"));

		QByteArray correct_xml_content =
			"<project>"
			"<name>Example_Project_Name</name>"
			"<aliases>Example;TestProject</aliases>"
			"<type>research</type>"
			"<internal_coordinator_id>1</internal_coordinator_id>"
			"<comment>Example project created through the API</comment>"
			"<analysis>variants</analysis>"
			"<preserve_fastqs>1</preserve_fastqs>"
			"<email_notification>alice@example.org;bob@example.org</email_notification>"
			"<archived>0</archived>"
			"<matchmaking>yes</matchmaking>"
			"</project>";

		DatabaseSchema db_schema = DatabaseSchema::loadFromDatabase(test_db);
		XmlImportValidator validator(db_schema);
		XmlValidationResult result = validator.validateInsert(correct_xml_content, "project");
		IS_TRUE(result.isValid());
		TableSchema table = db_schema.table("project");
		DatabaseInsert::insert(test_db, table, result.values);

		QByteArray incorrect_xml_content =
			"<project>"
			"<label>Project Label</label>"
			"<aliases>Example;TestProject</aliases>"
			"<type>research</type>"
			"<internal_coordinator_id>1</internal_coordinator_id>"
			"<comment>Example project created through the API</comment>"
			"<analysis>variants</analysis>"
			"<preserve_fastqs>1</preserve_fastqs>"
			"<email_notification>alice@example.org;bob@example.org</email_notification>"
			"<archived>0</archived>"
			"<matchmaking>yes</matchmaking>"
			"</project>";
		result = validator.validateInsert(incorrect_xml_content, "project");
		IS_TRUE(!result.isValid());
		IS_THROWN(DatabaseException, DatabaseInsert::insert(test_db, table, result.values));

		correct_xml_content =
			"<processing_system>"
			"<name_short>NovaSeq_WGS3</name_short>"
			"<name_manufacturer>Illumina NovaSeq 60003</name_manufacturer>"
			"<platform>Illumina</platform>"
			"<adapter1_p5>AGATCGGAAGAGCACACGTCTGAACTCCAGTCA</adapter1_p5>"
			"<adapter2_p7>AGATCGGAAGAGCGTCGTGTAGGGAAAGAGTGT</adapter2_p7>"
			"<type>WGS</type>"
			"<shotgun>1</shotgun>"
			"<umi_type>n/a</umi_type>"
			"<target_file>subpanel.bed</target_file>"
			"<genome_id>1</genome_id>"
			"</processing_system>";
		result = validator.validateInsert(correct_xml_content, "processing_system");
		IS_TRUE(result.isValid());
		table = db_schema.table("processing_system");
		DatabaseInsert::insert(test_db, table, result.values);

		incorrect_xml_content =
			"<processing_system>"
			"<name>NovaSeq_name</name>"
			"<name_manufacturer>Illumina NovaSeq 60003</name_manufacturer>"
			"<platform>illumina</platform>"
			"<adapter1_p5>AGATCGGAAGAGCACACGTCTGAACTCCAGTCA</adapter1_p5>"
			"<adapter2_p7>AGATCGGAAGAGCGTCGTGTAGGGAAAGAGTGT</adapter2_p7>"
			"<type>wgs</type>"
			"<shotgun>1</shotgun>"
			"<umi_type>n/a</umi_type>"
			"<target_file>subpanel.bed</target_file>"
			"<genome_id>1</genome_id>"
			"</processing_system>";
		result = validator.validateInsert(incorrect_xml_content, "processing_system");
		IS_TRUE(!result.isValid());
		IS_THROWN(DatabaseException, DatabaseInsert::insert(test_db, table, result.values));

		correct_xml_content =
			"<device>"
			"<type>NovaSeq6000</type>"
			"<name>NovaSeq6000_01</name>"
			"<comment>Main production sequencing instrument</comment>"
			"</device>";
		result = validator.validateInsert(correct_xml_content, "device");
		IS_TRUE(result.isValid());
		table = db_schema.table("device");
		DatabaseInsert::insert(test_db, table, result.values);

		incorrect_xml_content =
			"<device>"
			"<category>novaseq1000</category>"
			"<name>NovaSeq6000_01</name>"
			"</device>";
		result = validator.validateInsert(incorrect_xml_content, "device");
		IS_TRUE(!result.isValid());
		IS_THROWN(DatabaseException, DatabaseInsert::insert(test_db, table, result.values));

		correct_xml_content =
			"<sequencing_run>"
			"<name>RUN_2026_08_19_001</name>"
			"<fcid>H7ABCXY123</fcid>"
			"<flowcell_type>Illumina NovaSeq S4</flowcell_type>"
			"<start_date>2026-08-19</start_date>"
			"<end_date>2026-08-20</end_date>"
			"<device_id>1</device_id>"
			"<side>n/a</side>"
			"<recipe>2x150+2x10+2x10</recipe>"
			"<pool_molarity>10.5</pool_molarity>"
			"<pool_quantification_method>qPCR</pool_quantification_method>"
			"<comment>Example sequencing run</comment>"
			"<quality>good</quality>"
			"<status>run_finished</status>"
			"<backup_done>1</backup_done>"
			"</sequencing_run>";
		result = validator.validateInsert(correct_xml_content, "sequencing_run");
		IS_TRUE(result.isValid());
		table = db_schema.table("sequencing_run");
		DatabaseInsert::insert(test_db, table, result.values);

		incorrect_xml_content =
			"<sequencing_run>"
			"<name>RUN_2026_08_19_001</name>"
			"<fcid>H7ABCXY123</fcid>"
			"<type>illumina novaseq s4</type>"
			"<start_date>2026-08-19</start_date>"
			"<end_date>2026-08-20</end_date>"
			"<device_id>1</device_id>"
			"<side>n/a</side>"
			"<recipe>2x150+2x10+2x10</recipe>"
			"<pool_molarity>10,5</pool_molarity>"
			"<pool_quantification_method>qpcr</pool_quantification_method>"
			"<comment>Example sequencing run</comment>"
			"<quality>good</quality>"
			"<status>run_finished</status>"
			"<backup_done>1</backup_done>"
			"</sequencing_run>";
		result = validator.validateInsert(incorrect_xml_content, "sequencing_run");
		IS_TRUE(!result.isValid());
		IS_THROWN(DatabaseException, DatabaseInsert::insert(test_db, table, result.values));

		correct_xml_content =
			"<sample>"
			"<name>SAMPLE_001</name>"
			"<name_external>ExternalSample001</name_external>"
			"<patient_identifier>PATIENT_001</patient_identifier>"
			"<received>2026-08-19</received>"
			"<receiver_id>1</receiver_id>"
			"<sample_type>DNA</sample_type>"
			"<tissue>blood</tissue>"
			"<species_id>1</species_id>"
			"<concentration>25.5</concentration>"
			"<volume>50.0</volume>"
			"<od_260_280>1.85</od_260_280>"
			"<gender>male</gender>"
			"<comment>Example DNA sample</comment>"
			"<quality>good</quality>"
			"<od_260_230>2.05</od_260_230>"
			"<integrity_number>8.7</integrity_number>"
			"<tumor>0</tumor>"
			"<ffpe>0</ffpe>"
			"<sender_id>1</sender_id>"
			"<disease_group>n/a</disease_group>"
			"<disease_status>n/a</disease_status>"
			"<year_of_birth>1985</year_of_birth>"
			"<order_date>2026-08-15</order_date>"
			"<sampling_date>2026-08-18</sampling_date>"
			"</sample>";
		result = validator.validateInsert(correct_xml_content, "sample");
		IS_TRUE(result.isValid());
		table = db_schema.table("sample");
		DatabaseInsert::insert(test_db, table, result.values);

		incorrect_xml_content =
			"<sample>"
			"<sample_name>SAMPLE_001</sample_name>"
			"<name_external>ExternalSample001</name_external>"
			"<patient_identifier>PATIENT_001</patient_identifier>"
			"<received>2026-08-19</received>"
			"<receiver_id>1</receiver_id>"
			"<sample_type>dna</sample_type>"
			"<tissue>blood</tissue>"
			"<species_id>1</species_id>"
			"<concentration>25.5</concentration>"
			"<volume>50.0</volume>"
			"<od_260_280>1.85</od_260_280>"
			"<gender>male</gender>"
			"<comment>Example DNA sample</comment>"
			"<quality>good</quality>"
			"<od_260_230>2,05</od_260_230>"
			"<integrity_number>8.7</integrity_number>"
			"<tumor>0</tumor>"
			"<sampling_date>2026-08-18</sampling_date>"
			"</sample>";
		result = validator.validateInsert(incorrect_xml_content, "sample");
		IS_TRUE(!result.isValid());
		IS_THROWN(DatabaseException, DatabaseInsert::insert(test_db, table, result.values));

		correct_xml_content =
			"<processed_sample>"
			"<sample_id>1</sample_id>"
			"<process_id>1</process_id>"
			"<sequencing_run_id>1</sequencing_run_id>"
			"<lane>1,2</lane>"
			"<mid1_i7>1</mid1_i7>"
			"<mid2_i5>2</mid2_i5>"
			"<operator_id>1</operator_id>"
			"<processing_system_id>1</processing_system_id>"
			"<comment>Example processed sample</comment>"
			"<project_id>1</project_id>"
			"<processing_input>25.0</processing_input>"
			"<molarity>10.5</molarity>"
			"<processing_modus>Biomek i5</processing_modus>"
			"<batch_number>BATCH_2026_08_19_001</batch_number>"
			"<quality>good</quality>"
			"<folder_override>/data/projects/example/sample</folder_override>"
			"<folder_override_client>/projects/example/sample</folder_override_client>"
			"<scheduled_for_resequencing>0</scheduled_for_resequencing>"
			"<urgent>0</urgent>"
			"</processed_sample>";
		result = validator.validateInsert(correct_xml_content, "processed_sample");
		IS_TRUE(result.isValid());
		table = db_schema.table("processed_sample");
		DatabaseInsert::insert(test_db, table, result.values);

		incorrect_xml_content =
			"<processed_sample>"
			"<mid2_i5>2</mid2_i5>"
			"<operator_id>1</operator_id>"
			"<processing_system_id>1</processing_system_id>"
			"<comment>Example processed sample</comment>"
			"<project_id>1</project_id>"
			"<processing_input>25,0</processing_input>"
			"<molarity>10,5</molarity>"
			"<processing_modus>biomek i5</processing_modus>"
			"<batch_number>BATCH_2026_08_19_001</batch_number>"
			"<quality>good</quality>"
			"<folder_override>/data/projects/example/sample</folder_override>"
			"<folder_override_client>/projects/example/sample</folder_override_client>"
			"<scheduled_for_resequencing>false</scheduled_for_resequencing>"
			"<urgent>false</urgent>"
			"</processed_sample>";
		result = validator.validateInsert(incorrect_xml_content, "processed_sample");
		IS_TRUE(!result.isValid());
		IS_THROWN(DatabaseException, DatabaseInsert::insert(test_db, table, result.values));

		correct_xml_content =
			"<sender>"
			"<name>Dr. Jane Smith</name>"
			"<phone>+497071123456</phone>"
			"<email>jane.smith@example.org</email>"
			"<affiliation>UKT</affiliation>"
			"</sender>";
		result = validator.validateInsert(correct_xml_content, "sender");
		IS_TRUE(result.isValid());
		table = db_schema.table("sender");
		DatabaseInsert::insert(test_db, table, result.values);

		incorrect_xml_content =
			"<sender>"
			"<phone>+497071123456</phone>"
			"<id>1</id>"
			"<email>jane.smith@example.org</email>"
			"<affiliation>UKT</affiliation>"
			"</sender>";
		result = validator.validateInsert(incorrect_xml_content, "sender");
		IS_TRUE(!result.isValid());
		IS_THROWN(DatabaseException, DatabaseInsert::insert(test_db, table, result.values));
	}
};
