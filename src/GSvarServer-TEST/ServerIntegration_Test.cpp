#ifndef SERVERINTEGRATIONTEST_H
#define SERVERINTEGRATIONTEST_H

#include "TestFramework.h"
#include "ServerHelper.h"
#include "HttpRequestHandler.h"
#include "VersatileFile.h"
#include "ServerDB.h"
#include "ClientHelper.h"
#include "BamReader.h"
#include "Statistics.h"
#include "BasicServer.h"

#include "QueuingEngineApiHelper.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkProxy>

ServerReply sendGetRequest(QByteArray& reply, QString url, HttpHeaders headers)
{
	ServerReply server_reply;
    try
	{
		server_reply = HttpRequestHandler(QNetworkProxy(QNetworkProxy::NoProxy)).get(url, headers);
		reply = server_reply.body;
	}
    catch(HttpException& e)
    {
		server_reply.body = e.body();
		server_reply.headers = e.headers();
		server_reply.status_code = e.status_code();
		Log::error(e.message() + ": code " + QString::number(e.status_code()));
	}
	return server_reply;
}

ServerReply sendPostRequest(QByteArray& reply, QString url, QByteArray data, HttpHeaders headers)
{
	ServerReply server_reply;
	try
	{
		server_reply = HttpRequestHandler(QNetworkProxy(QNetworkProxy::NoProxy)).post(url, data, headers);
		reply = server_reply.body;
	}
	catch(HttpException& e)
	{
		server_reply.body = e.body();
		server_reply.headers = e.headers();
		server_reply.status_code = e.status_code();
		Log::error(e.message() + ": code " + QString::number(e.status_code()));
	}
	return server_reply;
}

TEST_CLASS(Server_Integration_Test)
{
private:

	TEST_METHOD(test_if_server_is_running)
	{
		if (!ServerHelper::settingsValid(true))
		{
			SKIP("Server has not been configured correctly");
		}
		ServerDB().reinitializeDb();

		QByteArray reply;
		HttpHeaders add_headers;
		add_headers.insert("Accept", "text/html");
        add_headers.insert("Content-Type", "text/html");
		int code = sendGetRequest(reply, ClientHelper::serverApiUrl(), add_headers).status_code;
        if (code == 0)
		{
			SKIP("This test requieres a running server");
		}
		IS_TRUE(!reply.isEmpty());
	}

	TEST_METHOD(test_partial_content_multirange_request)
	{
		if (!ServerHelper::settingsValid(true))
		{
			SKIP("Server has not been configured correctly");
		}

		QByteArray reply;
		HttpHeaders add_headers;
		add_headers.insert("Accept", "text/html");
        add_headers.insert("Content-Type", "text/html");
		add_headers.insert("Range", "bytes=114-140,399-430");
		int code = sendGetRequest(reply, ClientHelper::serverApiUrl(), add_headers).status_code;
        if (code == 0)
		{
			SKIP("This test requieres a running server");
		}
        I_EQUAL(code, 206);
		IS_TRUE(reply.contains("Welcome to the GSvar server"));
		IS_TRUE(reply.contains("help"));
	}

	TEST_METHOD(test_partial_content_empty_end_request)
	{
		if (!ServerHelper::settingsValid(true))
		{
			SKIP("Server has not been configured correctly");
		}

		QByteArray reply;
		HttpHeaders add_headers;
		add_headers.insert("Accept", "text/html");
        add_headers.insert("Content-Type", "text/html");
        add_headers.insert("Range", "bytes=461-");
		int code = sendGetRequest(reply, ClientHelper::serverApiUrl(), add_headers).status_code;
        if (code == 0)
		{
			SKIP("This test requieres a running server");
		}
        I_EQUAL(code, 206);
		S_EQUAL(reply.trimmed(), "</html>");
	}

	TEST_METHOD(test_partial_content_empty_start_request)
	{
		if (!ServerHelper::settingsValid(true))
		{
			SKIP("Server has not been configured correctly");
		}

		QByteArray reply;
		HttpHeaders add_headers;
		add_headers.insert("Accept", "text/html");
        add_headers.insert("Content-Type", "text/html");
		add_headers.insert("Range", "bytes=-8");
		int code = sendGetRequest(reply, ClientHelper::serverApiUrl(), add_headers).status_code;
        if (code == 0)
		{
			SKIP("This test requieres a running server");
		}
        I_EQUAL(code, 206);
		S_EQUAL(reply.trimmed(), "</html>");

		add_headers.clear();
		add_headers.insert("Accept", "text/html");
        add_headers.insert("Content-Type", "text/html");
		add_headers.insert("Range", "bytes=0-5,5-8");
        IS_THROWN(HttpException, HttpRequestHandler(QNetworkProxy(QNetworkProxy::NoProxy)).get(ClientHelper::serverApiUrl(), add_headers));
	}	

	TEST_METHOD(test_token_based_authentication)
	{
		if (!ServerHelper::settingsValid(true))
		{
			SKIP("Server has not been configured correctly");
		}

		QByteArray reply;
		HttpHeaders add_headers;
		add_headers.insert("Accept", "text/html");
        add_headers.insert("Content-Type", "application/x-www-form-urlencoded");
        QByteArray data = "name=ahmustm1&password=123456";
        int code = 200;

        try
        {
            ServerReply server_reply = HttpRequestHandler(QNetworkProxy(QNetworkProxy::NoProxy)).post(ClientHelper::serverApiUrl() + "login", data, add_headers);
            code = server_reply.status_code;
            reply = server_reply.body;
        }
        catch(HttpException& e)
        {
            code = e.status_code();
        }

        if (code == 0)
        {
            SKIP("This test requieres a running server");
        }
        I_EQUAL(code, 200);

		QString token = reply.trimmed();
		IS_TRUE(!token.isEmpty());

		reply.clear();
		add_headers.clear();
		add_headers.insert("Accept", "application/json");
        add_headers.insert("Content-Type", "application/json");
		code = sendGetRequest(reply, ClientHelper::serverApiUrl() + "session?token=" + token, add_headers).status_code;
		QJsonDocument session_json = QJsonDocument::fromJson(reply);
		IS_TRUE(session_json.isObject());

		reply.clear();
		add_headers.insert("Authorization", "Bearer "+token.toUtf8());
		code = sendGetRequest(reply, ClientHelper::serverApiUrl() + "session", add_headers).status_code;
		session_json = QJsonDocument::fromJson(reply);
		IS_TRUE(session_json.isObject());
		bool is_db_token = session_json.object().value("is_db_token").toBool();
		IS_FALSE(is_db_token);
	}

	TEST_METHOD(test_access_to_remote_bam_files)
	{
		if (!ServerHelper::settingsValid(true))
		{
			SKIP("Server has not been configured correctly");
		}

        QString filename = ClientHelper::serverApiUrl() + "assets/rna.bam";

		QByteArray reply;
		HttpHeaders add_headers;
		add_headers.insert("Accept", "application/octet-stream");
        add_headers.insert("Content-Type", "application/octet-stream");
		int code = sendGetRequest(reply, filename, add_headers).status_code;
        if (code == 0)
		{
			SKIP("This test requieres a running server");
		}

		// Read the BAM file, if the server is running
		BamReader reader(filename);
		I_EQUAL(reader.headerLines().count(), 2588);
		I_EQUAL(reader.chromosomes().count(), 2580);
	}

	TEST_METHOD(test_server_info_retrieval)
	{
		if (!ServerHelper::settingsValid(true))
		{
			SKIP("Server has not been configured correctly");
		}

		QByteArray reply;
		HttpHeaders add_headers;
		add_headers.insert("Accept", "application/json");
        add_headers.insert("Content-Type", "application/json");
		int code = sendGetRequest(reply, ClientHelper::serverApiUrl() + "info", add_headers).status_code;
        if (code == 0)
		{
			SKIP("This test requieres a running server");
		}

		QJsonDocument doc = QJsonDocument::fromJson(reply);
		IS_TRUE(doc.isObject());
		IS_TRUE(doc.object().contains("version"));
		IS_TRUE(doc.object().contains("api_version"));
		S_EQUAL(doc.object()["api_version"].toString(), "v1");
		IS_TRUE(doc.object().contains("start_time"));
		IS_TRUE(doc.object().contains("htslib_version"));
		IS_TRUE(doc.object()["htslib_version"].toString().startsWith("1."));
	}

	TEST_METHOD(test_client_info_retrieval)
	{
		if (!ServerHelper::settingsValid(true))
		{
			SKIP("Server has not been configured correctly");
		}

		QByteArray reply;
		HttpHeaders add_headers;
		add_headers.insert("Accept", "application/json");
        add_headers.insert("Content-Type", "application/json");
		int code = sendGetRequest(reply, ClientHelper::serverApiUrl() + "current_client", add_headers).status_code;
        if (code == 0)
		{
			SKIP("This test requieres a running server");
		}

		QJsonDocument out = QJsonDocument::fromJson(reply);
		IS_TRUE(out.isObject());
	}

	TEST_METHOD(test_remote_file_metadata)
    {
		if (!ServerHelper::settingsValid(true))
        {
            SKIP("Server has not been configured correctly");
        }
        const QString bam_file = ClientHelper::serverApiUrl() + "assets/rna.bam";

        QByteArray reply;
        HttpHeaders add_headers;
        add_headers.insert("Accept", "text/html");
        add_headers.insert("Content-Type", "text/html");
        add_headers.insert("Range", "bytes=-8");
		int code = sendGetRequest(reply, ClientHelper::serverApiUrl(), add_headers).status_code;
        if (code == 0)
        {
            SKIP("This test requieres a running server");
        }

        QSharedPointer<VersatileFile> file_over_https(new VersatileFile(bam_file));
        IS_TRUE(file_over_https->exists());
        S_EQUAL(file_over_https->fileName(), bam_file)
        I_EQUAL(file_over_https->size(), 117570);
        IS_TRUE(file_over_https->isReadable());
    }

    TEST_METHOD(test_remote_text_file_random_access)
    {
		if (!ServerHelper::settingsValid(true))
        {
            SKIP("Server has not been configured correctly");
        }

        const QString html_file = ClientHelper::serverApiUrl();

        QFile *local_asset_file = new QFile(":/assets/client/info.html");
        local_asset_file->open(QIODevice::ReadOnly);
        QByteArray asset_file_content = local_asset_file->readAll();

        QByteArray reply;
        HttpHeaders add_headers;
        add_headers.insert("Accept", "text/html");
        add_headers.insert("Content-Type", "text/html");
        add_headers.insert("Range", "bytes=-8");
		int code = sendGetRequest(reply, ClientHelper::serverApiUrl(), add_headers).status_code;
        if (code == 0)
        {
            SKIP("This test requieres a running server");
        }

        QSharedPointer<VersatileFile> index_page_file(new VersatileFile(html_file));
        index_page_file.data()->open(QIODevice::ReadOnly);
        QByteArray index_page_content = index_page_file->readAll();
        S_EQUAL(asset_file_content, index_page_content);

        IS_TRUE(index_page_file->atEnd());
        index_page_file->seek(0);
        IS_FALSE(index_page_file->atEnd());
        QByteArray first_line = index_page_file->readLine();
        S_EQUAL(first_line.trimmed(), "<!doctype html>");
        I_EQUAL(index_page_file->pos(), 16);
        index_page_file->seek(10);
        QByteArray line_fragment = index_page_file->read(4);
        S_EQUAL(line_fragment, "html");
        I_EQUAL(index_page_file->pos(), 14);
    }

    TEST_METHOD(test_remote_gender_xy)
    {
        if (!ServerHelper::settingsValid(true))
        {
            SKIP("Server has not been configured correctly");
        }

        QByteArray reply;
        HttpHeaders add_headers;
        add_headers.insert("Accept", "text/html");
        add_headers.insert("Content-Type", "text/html");
        add_headers.insert("Range", "bytes=-8");
		int code = sendGetRequest(reply, ClientHelper::serverApiUrl(), add_headers).status_code;
        if (code == 0)
        {
            SKIP("This test requieres a running server");
        }

        QString gender_filename = ClientHelper::serverApiUrl() + "assets/panel.bam";
        GenderEstimate estimate = Statistics::genderXY(gender_filename);
        I_EQUAL(estimate.add_info.count(), 3);
        S_EQUAL(estimate.add_info[0].key, "reads_chry");
        S_EQUAL(estimate.add_info[0].value, "0");
        S_EQUAL(estimate.add_info[1].key, "reads_chrx");
        S_EQUAL(estimate.add_info[1].value, "30528");
        S_EQUAL(estimate.add_info[2].key, "ratio_chry_chrx");
        S_EQUAL(estimate.add_info[2].value, "0.0000");
        S_EQUAL(estimate.gender, "female");
    }

    TEST_METHOD(test_versatile_file_url_gz)
    {
        if (!ServerHelper::settingsValid(true))
        {
            SKIP("Server has not been configured correctly");
        }

        QByteArray reply;
        HttpHeaders add_headers;
        add_headers.insert("Accept", "text/html");
        add_headers.insert("Content-Type", "text/html");
        add_headers.insert("Range", "bytes=-8");
		int code = sendGetRequest(reply, ClientHelper::serverApiUrl(), add_headers).status_code;
        if (code == 0)
        {
            SKIP("This test requieres a running server");
        }

        QString filename_small = ClientHelper::serverApiUrl() + "assets/ancestry_hg38.vcf.gz";
        AncestryEstimates ancestry_small = Statistics::ancestry(GenomeBuild::HG38, filename_small);
        I_EQUAL(ancestry_small.snps, 2126);
        F_EQUAL2(ancestry_small.afr, 0.4984, 0.001);
        F_EQUAL2(ancestry_small.eur, 0.0241, 0.001);
        F_EQUAL2(ancestry_small.sas, 0.1046, 0.001);
        F_EQUAL2(ancestry_small.eas, 0.0742, 0.001);
        S_EQUAL(ancestry_small.population, "AFR");

        QString filename_large = ClientHelper::serverApiUrl() + "assets/NA12878_58_var_annotated.vcf.gz";
        AncestryEstimates ancestry_large = Statistics::ancestry(GenomeBuild::HG38, filename_large);
        // AncestryEstimates ancestry = Statistics::ancestry(GenomeBuild::HG38, TESTDATA("data/NA12878_58_var_annotated.vcf.gz"));

        I_EQUAL(ancestry_large.snps, 2907);
        F_EQUAL2(ancestry_large.afr, 0.00860332, 0.001);
        F_EQUAL2(ancestry_large.eur, 0.310719, 0.001);
        F_EQUAL2(ancestry_large.sas, 0.156908, 0.001);
        F_EQUAL2(ancestry_large.eas, 0.0452005, 0.001);
        S_EQUAL(ancestry_large.population, "EUR");
    }

    TEST_METHOD(plain_text_file)
    {
        if (!ServerHelper::settingsValid(true))
        {
            SKIP("Server has not been configured correctly");
        }

        QByteArray reply;
        HttpHeaders add_headers;
        add_headers.insert("Accept", "text/html");
        add_headers.insert("Content-Type", "text/html");
        add_headers.insert("Range", "bytes=-8");
		int code = sendGetRequest(reply, ClientHelper::serverApiUrl(), add_headers).status_code;
        if (code == 0)
        {
            SKIP("This test requieres a running server");
        }

        QString text_filename = ClientHelper::serverApiUrl() + "assets/txt_file.txt";

        VersatileFile file(text_filename);
        file.open();
        I_EQUAL(file.mode(), VersatileFile::URL);

        QString entire_file = file.readAll();
        S_EQUAL(entire_file, "##comment\n#header\nthis is a plain text file");
        IS_TRUE(file.atEnd())
        file.seek(0);

        IS_FALSE(file.atEnd())
        QString line = file.readLine();
        S_EQUAL(line.trimmed(), "##comment");
        I_EQUAL(file.pos(), 10);

        QString fragment = file.read(7);
        S_EQUAL(fragment, "#header");
        I_EQUAL(file.pos(), 17);

        file.seek(0);
        QString start_fragment = file.read(9);
        S_EQUAL(start_fragment, "##comment");
        I_EQUAL(file.pos(), 9);
    }

    TEST_METHOD(gzipped_text_file)
    {
        if (!ServerHelper::settingsValid(true))
        {
            SKIP("Server has not been configured correctly");
        }

        QByteArray reply;
        HttpHeaders add_headers;
        add_headers.insert("Accept", "text/html");
        add_headers.insert("Content-Type", "text/html");
        add_headers.insert("Range", "bytes=-8");
		int code = sendGetRequest(reply, ClientHelper::serverApiUrl(), add_headers).status_code;
        if (code == 0)
        {
            SKIP("This test requieres a running server");
        }

        QString text_gz_filename = ClientHelper::serverApiUrl() + "assets/txt_file_gzipped.txt.gz";

        VersatileFile file(text_gz_filename);
        file.open();
        I_EQUAL(file.mode(), VersatileFile::URL_GZ);

        QString entire_file = file.readAll();
        QString expected_content = "##comment\r\n#header\r\nthis is a gzipped text file";
        S_EQUAL(entire_file.trimmed(), expected_content);
        IS_TRUE(file.atEnd())
        file.seek(0);

        IS_FALSE(file.atEnd());
        QString line = file.readLine();
        IS_FALSE(file.atEnd());
        S_EQUAL(line.trimmed(), "##comment");

        while(!file.atEnd())
        {
            line = file.readLine();
        }
        IS_TRUE(file.atEnd());
    }

	TEST_METHOD(test_queuing_engine_api_endpoints)
	{
		if (Settings::string("qe_api_base_url", true).isEmpty())
		{
			SKIP("This test requieres a queuing engine API server base URL, which is not set in the config");
		}

		QUrl api_server_url = QUrl(Settings::string("qe_api_base_url", true));
		int api_server_port = api_server_url.port();

		BasicServer server(api_server_port);
		server.addRoute("/info",  QHttpServerRequest::Method::Get, [] () {
			QJsonObject obj{
				{"info", "This is a test"}
			};

			return QHttpServerResponse("application/json", QJsonDocument(obj).toJson(QJsonDocument::Compact));
		});

		server.addRoute("/jobs",  QHttpServerRequest::Method::Post, [] (const QHttpServerRequest &req) {
			QJsonDocument doc = QJsonDocument::fromJson(req.body());
			QJsonObject error_reply;


			if (!doc.isObject())
			{
				error_reply.insert("result", "Your input is not a valid JSON object");
				return QHttpServerResponse("application/json", QJsonDocument(error_reply).toJson(QJsonDocument::Compact), QHttpServerResponder::StatusCode::InternalServerError);
			}
			QJsonObject top_level_obj = doc.object();
			if (!top_level_obj.contains("action"))
			{
				error_reply.insert("result", "Action is not specified");
				return QHttpServerResponse("application/json", QJsonDocument(error_reply).toJson(QJsonDocument::Compact), QHttpServerResponder::StatusCode::InternalServerError);
			}
			if (!top_level_obj.contains("token"))
			{
				error_reply.insert("result", "Secutiry token has not been provided");
				return QHttpServerResponse("application/json", QJsonDocument(error_reply).toJson(QJsonDocument::Compact), QHttpServerResponder::StatusCode::Unauthorized);
			}
			QStringList allowed_actions = QStringList() << "submit" << "update" << "check" << "delete";
			if (!allowed_actions.contains(top_level_obj.value("action").toString()))
			{
				error_reply.insert("result", "Action '" + top_level_obj.value("action").toString() + "' is not allowed");
				return QHttpServerResponse("application/json", QJsonDocument(error_reply).toJson(QJsonDocument::Compact), QHttpServerResponder::StatusCode::InternalServerError);
			}

			QJsonObject success_reply;
			QStringList required_fields;
			if (top_level_obj.value("action").toString() == "submit")
			{
				required_fields = QStringList() << "threads" << "queues" << "script_args" << "working_directory" << "script";
			}

			if (top_level_obj.value("action").toString() == "update")
			{
				required_fields = QStringList() << "qe_job_id" << "qe_job_queue";
				success_reply.insert("status", "queued/running");
				success_reply.insert("queue", "queue1");
			}

			if (top_level_obj.value("action").toString() == "check")
			{
				required_fields = QStringList() << "qe_job_id" << "stdout_stderr";
				success_reply.insert("qe_exit_code", 0);
			}

			if (top_level_obj.value("action").toString() == "delete")
			{
				required_fields = QStringList() << "qe_job_id" << "qe_job_type";
			}

			for (QString field: required_fields)
			{
				if (!top_level_obj.contains(field))
				{
					error_reply.insert("result", "JSON does not contain '" + field + "' field");
					return QHttpServerResponse("application/json", QJsonDocument(error_reply).toJson(QJsonDocument::Compact), QHttpServerResponder::StatusCode::InternalServerError);
				}
			}

			success_reply.insert("result", top_level_obj.value("action").toString() + " action has been successful!");
			success_reply.insert("qe_job_id", "1");
			success_reply.insert("exit_code", 0);
			return QHttpServerResponse("application/json", QJsonDocument(success_reply).toJson(QJsonDocument::Compact));
		});

		QByteArray reply;

		ServerReply api_reply = sendGetRequest(reply, api_server_url.toString() + "/info", HttpHeaders{});
		int status_code = api_reply.status_code;
		I_EQUAL(status_code, 200);

		QJsonDocument response_doc = QJsonDocument::fromJson(reply);
		IS_TRUE(response_doc.isObject());
		IS_TRUE(response_doc.object().contains("info"));
		S_EQUAL(response_doc.object().value("info").toString(), "This is a test");

		int threads = 4;
		QStringList queues = QStringList() << "queue1" << "queue2" << "queue3";
		QStringList pipeline_args =  QStringList() << "arg1" << "arg2" << "arg3";
		QString project_folder = "CurrentProject";
		QString script = "pipeline.php";		
		QString security_token = "random_characters";

		QueuingEngineApiHelper api_helper = QueuingEngineApiHelper(api_server_url.toString() + "/jobs", QNetworkProxy::NoProxy, security_token);
		status_code = api_helper.submitJob(threads, queues, pipeline_args, project_folder, script).status_code;
		I_EQUAL(status_code, 200);

		QString qe_job_id = "1";
		QString qe_job_queue = "qe_job_queue_1";
		status_code = api_helper.updateRunningJob(qe_job_id, qe_job_queue).status_code;
		I_EQUAL(status_code, 200);

		QJsonDocument json_doc_output;
		QJsonObject top_level_json_object;
		top_level_json_object.insert("action", "fake");
		top_level_json_object.insert("token", security_token);
		top_level_json_object.insert("qe_job_id", qe_job_id);
		top_level_json_object.insert("qe_job_queue", qe_job_queue);		
		json_doc_output.setObject(top_level_json_object);
		HttpHeaders add_headers;
		add_headers.insert("Content-Type", "application/json");
		IS_THROWN(HttpException, HttpRequestHandler(QNetworkProxy::NoProxy).post(api_server_url.toString() + "/jobs", json_doc_output.toJson(), add_headers));

		ServerReply error_reply;
		try
		{
			error_reply = HttpRequestHandler(QNetworkProxy::NoProxy).post(api_server_url.toString() + "/jobs", json_doc_output.toJson(), add_headers);
		}
		catch(HttpException& e)
		{
			error_reply.body = e.body();
			error_reply.headers = e.headers();
			error_reply.status_code = e.status_code();
		}
		I_EQUAL(error_reply.status_code, 500);
		IS_TRUE(error_reply.body.contains("fake"));
		QJsonDocument error_response_doc = QJsonDocument::fromJson(error_reply.body);
		IS_TRUE(error_response_doc.isObject());
		IS_TRUE(error_response_doc.object().contains("result"));

		QJsonDocument json_doc_secure_output;
		QJsonObject top_level_secure_json_object;
		top_level_secure_json_object.insert("action", "fake");
		json_doc_secure_output.setObject(top_level_secure_json_object);
		try
		{
			error_reply = HttpRequestHandler(QNetworkProxy::NoProxy).post(api_server_url.toString() + "/jobs", json_doc_secure_output.toJson(), add_headers);
		}
		catch(HttpException& e)
		{
			error_reply.body = e.body();
			error_reply.headers = e.headers();
			error_reply.status_code = e.status_code();
		}
		I_EQUAL(error_reply.status_code, 401);

		QByteArrayList stdout_stderr;
		status_code = api_helper.checkCompletedJob(qe_job_id, stdout_stderr).status_code;
		I_EQUAL(status_code, 200);

		QString qe_job_type = "job_type";
		ServerReply delete_reply = api_helper.deleteJob(qe_job_id, qe_job_type);
		response_doc = QJsonDocument::fromJson(delete_reply.body);
		IS_TRUE(response_doc.isObject());
		IS_TRUE(response_doc.object().contains("result"));

		status_code = delete_reply.status_code;
		I_EQUAL(status_code, 200);

	}
};

#endif // SERVERINTEGRATIONTEST_H
