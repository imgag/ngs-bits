#include "TestWorker.h"
#include "ServerHelper.h"
#include "Exceptions.h"
#include "ClientHelper.h"
#include "BamReader.h"
#include "GenomeBuild.h"
#include <QNetworkProxy>

TestWorker::TestWorker()
    :QRunnable()
{
}

void TestWorker::run()
{
    Log::info("Starting a test worker instance");

    try
    {


        if (!ServerHelper::hasMinimalSettings())
        {
            Log::info("Server has not been configured correctly");
            return;
        }

        QByteArray reply;
        HttpHeaders add_headers;
        add_headers.insert("Accept", "text/html");
        add_headers.insert("Content-Type", "application/x-www-form-urlencoded");
        QByteArray data = "name=ahchera1&password=password";
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
            Log::info("This test requieres a running server");
            return;
        }
        if (code!=200)
        {
            THROW(Exception, "Error while logging in");
        }

        QString token = reply.trimmed();
        if (token.isEmpty())
        {
            THROW(Exception, "Empty token");
        }

        reply.clear();
        add_headers.clear();
        add_headers.insert("Accept", "application/json");
        add_headers.insert("Content-Type", "application/json");
        code = sendGetRequest(reply, ClientHelper::serverApiUrl() + "session?token=" + token, add_headers);
        QJsonDocument session_json = QJsonDocument::fromJson(reply);
        if (!session_json.isObject())
        {
            THROW(Exception, "Response does not contain a valid JSON object");
        }

        reply.clear();
        add_headers.insert("Authorization", "Bearer "+token.toUtf8());
        code = sendGetRequest(reply, ClientHelper::serverApiUrl() + "session", add_headers);
        session_json = QJsonDocument::fromJson(reply);
        if (!session_json.isObject())
        {
            THROW(Exception, "Response does not contain a valid JSON object");
        }

        bool is_db_token = session_json.object().value("is_db_token").toBool();
        if (is_db_token)
        {
            THROW(Exception, "Response contains a db token, a regular token was expected");
        }



        QString filename = ClientHelper::serverApiUrl() + "bam/rna.bam";

        reply.clear();
        add_headers.clear();
        add_headers.insert("Accept", "application/octet-stream");
        add_headers.insert("Content-Type", "application/octet-stream");
        code = sendGetRequest(reply, filename, add_headers);
        if (code == 0)
        {
            Log::info("This test requieres a running server");
            return;
        }

        // Read the BAM file, if the server is running
        try
        {
            BamReader reader(filename);
            Log::info(buildToString(reader.build()));
        }
        catch(Exception& e)
        {
            Log::info("!!!!!!!!!!!!!===============================");
            Log::info(e.message());
        }
        catch(...)
        {
            Log::error("Unexpected exception!!!");
        }
    }
    catch(Exception& e)
    {
        Log::info("!!!!!!!!!!!!!===============================");
        Log::info(e.message());
    }
    catch(...)
    {
        Log::error("Unexpected exception!!!");
    }
}

int TestWorker::sendGetRequest(QByteArray& reply, QString url, HttpHeaders headers)
{
    int status_code;
    try
    {
        ServerReply server_reply = HttpRequestHandler(QNetworkProxy(QNetworkProxy::NoProxy)).get(url, headers);
        reply = server_reply.body;
        status_code = server_reply.status_code;
    }
    catch(HttpException& e)
    {
        reply = e.body();
        status_code = e.status_code();

        Log::error(e.message());
    }
    return status_code;
}


