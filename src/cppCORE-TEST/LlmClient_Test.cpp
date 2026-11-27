#include "TestFramework.h"
#include "LlmClient.h"
#include <QHttpServer>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTcpServer>

namespace
{
	QUrl baseUrl(const QTcpServer& server, const QString& path = "/v1")
	{
		return QUrl(QString("http://127.0.0.1:%1%2").arg(server.serverPort()).arg(path));
	}
}

TEST_CLASS(LlmClient_Test)
{
public:
	TEST_METHOD(sends_chat_completion_and_reads_usage)
	{
		QHttpServer server;
		QTcpServer listener;
		IS_TRUE(listener.listen(QHostAddress::LocalHost));
		IS_TRUE(server.bind(&listener));

		QJsonObject payload;
		QByteArray authorization;
		QByteArray workspace;
		QByteArray content_type;
		server.route("/proxy/v1/chat/completions", QHttpServerRequest::Method::Post, [&](const QHttpServerRequest& request)
		{
			payload = QJsonDocument::fromJson(request.body()).object();
			authorization = request.value("Authorization");
			workspace = request.value("anthropic-workspace-id");
			content_type = request.value("Content-Type");
			return QHttpServerResponse("application/json",
				R"({"choices":[{"message":{"content":"Gr\u00fc\u00dfe"},"finish_reason":"stop"}],
					"usage":{"prompt_tokens":10,"completion_tokens":3,"total_tokens":13}})");
		});

		const LlmClient client({baseUrl(listener, "/proxy/v1/"), "test-model", "test-key",
			{{"anthropic-workspace-id", "test-workspace"}}, 3000});
		const QString prompt = QStringLiteral("Gr\u00fc\u00dfe\n\"test\"");
		const auto response = client.query(prompt, "Answer in German.");
		S_EQUAL(authorization, "Bearer test-key");
		S_EQUAL(workspace, "test-workspace");
		S_EQUAL(content_type, "application/json");
		S_EQUAL(payload.value("model").toString(), "test-model");
		const auto messages = payload.value("messages").toArray();
		I_EQUAL(messages.size(), 2);
		S_EQUAL(messages.at(0).toObject().value("role").toString(), "system");
		S_EQUAL(messages.at(0).toObject().value("content").toString(), "Answer in German.");
		S_EQUAL(messages.at(1).toObject().value("role").toString(), "user");
		S_EQUAL(messages.at(1).toObject().value("content").toString(), prompt);
		S_EQUAL(response.text, QStringLiteral("Gr\u00fc\u00dfe"));
		S_EQUAL(response.finish_reason, "stop");
		I_EQUAL(response.prompt_tokens.value_or(-1), 10);
		I_EQUAL(response.completion_tokens.value_or(-1), 3);
		I_EQUAL(response.total_tokens.value_or(-1), 13);
	}

	TEST_METHOD(handles_truncation_without_credentials_or_usage)
	{
		QHttpServer server;
		QTcpServer listener;
		IS_TRUE(listener.listen(QHostAddress::LocalHost));
		server.bind(&listener);
		IS_TRUE(server.servers().contains(&listener));

		bool has_authorization = false;
		qsizetype message_count = 0;
		server.route("/v1/chat/completions", QHttpServerRequest::Method::Post, [&](const QHttpServerRequest& request)
		{
			has_authorization = !request.value("Authorization").isEmpty();
			message_count = QJsonDocument::fromJson(request.body()).object().value("messages").toArray().size();
			return QHttpServerResponse("application/json",
				R"({"choices":[{"message":{"content":"partial answer"},"finish_reason":"length"}]})");
		});

		const LlmClient client({baseUrl(listener), "local-model", {}, {}, 3000});
		const auto response = client.query("Hello");
		IS_FALSE(has_authorization);
		I_EQUAL(message_count, 1);
		S_EQUAL(response.text, "partial answer");
		S_EQUAL(response.finish_reason, "length");
		IS_FALSE(response.total_tokens.has_value());
	}

	TEST_METHOD(reports_http_errors_without_response_body)
	{
		QHttpServer server;
		QTcpServer listener;
		IS_TRUE(listener.listen(QHostAddress::LocalHost));
		server.bind(&listener);
		IS_TRUE(server.servers().contains(&listener));
		server.route("/v1/chat/completions", QHttpServerRequest::Method::Post, []()
		{
			return QHttpServerResponse("text/plain", "private prompt", QHttpServerResponder::StatusCode::TooManyRequests);
		});

		const LlmClient client({baseUrl(listener), "test-model", "test-key", {}, 3000});
		try
		{
			client.query("private prompt");
		}
		catch (const HttpException& error)
		{
			I_EQUAL(error.status_code(), 429);
			IS_TRUE(error.body().isEmpty());
			IS_FALSE(error.message().contains("private prompt"));
			return;
		}
		IS_TRUE(false);
	}

	TEST_METHOD(rejects_invalid_response)
	{
		QHttpServer server;
		QTcpServer listener;
		IS_TRUE(listener.listen(QHostAddress::LocalHost));
		server.bind(&listener);
		IS_TRUE(server.servers().contains(&listener));
		server.route("/v1/chat/completions", QHttpServerRequest::Method::Post, []()
		{
			return QHttpServerResponse("application/json", R"({"choices":[]})");
		});

		const LlmClient client({baseUrl(listener), "test-model", {}, {}, 3000});
		IS_THROWN(FileParseException, client.query("Hello"));
	}

	TEST_METHOD(times_out_if_server_never_replies)
	{
		QTcpServer listener;
		IS_TRUE(listener.listen(QHostAddress::LocalHost));
		const LlmClient client({baseUrl(listener), "test-model", {}, {}, 100});
		try
		{
			client.query("Hello");
		}
		catch (const NetworkException& error)
		{
			S_EQUAL(error.message(), "LLM request timed out");
			return;
		}
		IS_TRUE(false);
	}
};
