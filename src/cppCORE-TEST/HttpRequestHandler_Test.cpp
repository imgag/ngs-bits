#include "TestFramework.h"
#include "HttpRequestHandler.h"
#include "BasicServer.h"
#include "Settings.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QUrl>

bool ignoreProxySettings()
{
	if (!qputenv("NO_PROXY", "localhost,127.0.0.1,::1")) return false;
	return true;
}

TEST_CLASS(HttpRequestHandler_Test)
{
private:
	TEST_METHOD(get_with_proxy)
	{
		//at UKT, we need proxy credentials...
		QString url = "https://raw.githubusercontent.com/imgag/ngs-bits/refs/heads/master/src/cppCORE-TEST/data_in/txt_file.txt";
		if (HttpRequestHandler::proxyForUrl(url).endsWith(".med.uni-tuebingen.de") && Settings::string("proxy_password").trimmed().isEmpty()) return;

		HttpRequestHandler handler;
		ServerReply reply = handler.get(url);
		I_EQUAL(reply.status_code, 200);
		S_EQUAL(reply.body, "##comment\n#header\nthis is a plain text file")
	}

	TEST_METHOD(head_for_external_file)
	{
		QString url = "https://download.imgag.de/public/genomes/GRCh38.fa.fai";
		if (HttpRequestHandler::proxyForUrl(url).endsWith(".med.uni-tuebingen.de") && Settings::string("proxy_password").trimmed().isEmpty()) return;

		HttpRequestHandler handler;
		HttpHeaders example_headers;
		ServerReply reply = handler.head(url, example_headers);
		I_EQUAL(reply.status_code, 200);
		IS_TRUE(reply.body.isEmpty());

		bool has_length_header = false;
		QByteArray header_name = "";
		for (auto it = reply.headers.cbegin(); it != reply.headers.cend(); ++it) {
			const QByteArray &key = it.key();
			if (key.toLower()=="content-length")
			{
				has_length_header = true;
				header_name = key;
			}
		}

		IS_TRUE(has_length_header);
		S_EQUAL(reply.headers.value(header_name), "123148");
	}

	TEST_METHOD(head_post_and_put_with_mock_server)
	{
		if (!ignoreProxySettings()) SKIP("Could not set NO_PROXY variable, the tests may not work properly");

		#if QT_VERSION >= QT_VERSION_CHECK(6, 5, 0)
		QString mock_server = Settings::string("mock_server_url", true);
		if (mock_server.isEmpty())
		{
			SKIP("This test requieres a mock server URL, which is not set in the config");
		}

		QUrl api_server_url = QUrl(mock_server);
		int api_server_port = api_server_url.port();
		BasicServer server(api_server_port);
		server.addRoute("/size",  QHttpServerRequest::Method::Head, [] () {
			QHttpServerResponse response(QHttpServerResponder::StatusCode::Ok);
			QHttpHeaders headers;
			headers.append(QHttpHeaders::WellKnownHeader::ContentType, "text/plain");
			// custom metadata instead of Content-Length (QHttpHeaders adds the second Content-Length header with the value 0, if the body is empty)
			headers.append("x-file-size", QByteArray::number(77));
			response.setHeaders(std::move(headers));
			return response;
		});

		server.addRoute("/edit_data",  QHttpServerRequest::Method::Put, [] (const QHttpServerRequest &request)
		{
			const QByteArray body = request.body();
			if (body == "correct upload data")
			{
				QJsonObject obj{{"message", "File has been uploaded"}};
				return QHttpServerResponse("application/json", QJsonDocument(obj).toJson(QJsonDocument::Compact));
			}
			return QHttpServerResponse(QHttpServerResponder::StatusCode::BadRequest);
		});

		server.addRoute("/submit_form_data",  QHttpServerRequest::Method::Post, [] (const QHttpServerRequest &request)
		{
			const QByteArray body = request.body();
			if (body == "correct form data")
			{
				QJsonObject obj{{"message", "Form has been submited"}};
				return QHttpServerResponse("application/json", QJsonDocument(obj).toJson(QJsonDocument::Compact));
			}
			return QHttpServerResponse(QHttpServerResponder::StatusCode::BadRequest);
		});

		server.addRoute("/submit_multipart_form_data", QHttpServerRequest::Method::Post, [] (const QHttpServerRequest &request)
		{
			const QByteArray content_type = request.value("Content-Type");
			if (!content_type.contains("multipart/form-data")) return QHttpServerResponse(QHttpServerResponder::StatusCode::BadRequest);

			// Extract boundary
			const QByteArray boundary_prefix = "boundary=";
			int boundary_pos = content_type.indexOf(boundary_prefix);
			if (boundary_pos == -1) return QHttpServerResponse(QHttpServerResponder::StatusCode::BadRequest);

			QByteArray boundary = "--" + content_type.mid(boundary_pos + boundary_prefix.size());
			QByteArray body = request.body();
			QList<QByteArray> parts = body.split('\n');
			QString field_value;
			bool has_field = false;

			for (const QByteArray &line : parts)
			{
				if (line.startsWith(boundary))
				{
					has_field = false;
					continue;
				}

				if (line.contains("name=\"my_field\""))
				{
					has_field = true;
					continue;
				}

				// Empty line separates headers from content
				if (has_field && line.trimmed().isEmpty()) continue;

				if (has_field)
				{
					field_value = QString::fromUtf8(line.trimmed());
					break;
				}
			}

			if (field_value == "correct form data")
			{
				QJsonObject obj{
					{"message", "Multipart form has been submitted"}
				};

				return QHttpServerResponse("application/json", QJsonDocument(obj).toJson(QJsonDocument::Compact));
			}

			return QHttpServerResponse(QHttpServerResponder::StatusCode::BadRequest);
		});

		HttpRequestHandler handler;
		HttpHeaders headers;
		ServerReply reply = handler.head(api_server_url.toString() + "/size", headers);
		QByteArray payload = "correct upload data";

		I_EQUAL(reply.status_code, 200);
		IS_TRUE(reply.body.isEmpty());
		IS_TRUE(reply.headers.contains("x-file-size"));
		I_EQUAL(reply.headers["x-file-size"].toLongLong(), 77);

		headers.clear();
		headers.insert("Content-Type", "application/octet-stream");

		reply = handler.put(api_server_url.toString() + "/edit_data", payload, headers);
		I_EQUAL(reply.status_code, 200);
		QJsonDocument put_doc = QJsonDocument::fromJson(reply.body);
		IS_TRUE(put_doc.isObject());
		IS_TRUE(put_doc.object().contains("message"));
		S_EQUAL(put_doc.object().value("message").toString(), "File has been uploaded");

		payload = "wrong payload";
		IS_THROWN(HttpException, handler.put(api_server_url.toString() + "/edit_data", payload, headers));

		payload = "correct form data";
		reply = handler.post(api_server_url.toString() + "/submit_form_data", payload, headers);
		I_EQUAL(reply.status_code, 200);
		QJsonDocument post_doc = QJsonDocument::fromJson(reply.body);
		IS_TRUE(post_doc.isObject());
		IS_TRUE(post_doc.object().contains("message"));
		S_EQUAL(post_doc.object().value("message").toString(), "Form has been submited");

		payload = "wrong payload";
		IS_THROWN(HttpException, handler.post(api_server_url.toString() + "/submit_form_data", payload, headers));

		headers.clear();
		headers.insert("Content-Type", "Content-Type: multipart/form-data; boundary=\"bndr\"");
		QHttpMultiPart *multipart_form = new QHttpMultiPart(QHttpMultiPart::FormDataType);
		multipart_form->setBoundary("bndr");
		QHttpPart field_part;
		field_part.setHeader(QNetworkRequest::ContentDispositionHeader, QVariant("form-data; name=\"my_field\""));
		field_part.setBody("correct form data");
		multipart_form->append(field_part);

		reply = handler.post(api_server_url.toString() + "/submit_multipart_form_data", multipart_form, headers);
		I_EQUAL(reply.status_code, 200);
		QJsonDocument multipart_post_doc = QJsonDocument::fromJson(reply.body);
		IS_TRUE(multipart_post_doc.isObject());
		IS_TRUE(multipart_post_doc.object().contains("message"));
		S_EQUAL(multipart_post_doc.object().value("message").toString(), "Multipart form has been submitted");

		#else
		SKIP("Your Qt version does not support QtHttpServer, this test cannot be started!");
		#endif
	}
};