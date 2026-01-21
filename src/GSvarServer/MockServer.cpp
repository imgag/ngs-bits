#include "MockServer.h"
#include "Log.h"


#include <QJsonObject>
#include <QJsonDocument>

MockServer::MockServer(quint16 port, QObject* parent)
	: QObject(parent)
{
	setupRoutes();

	if (m_tcpServer.listen(QHostAddress::Any, port)) {
		m_httpServer.bind(&m_tcpServer);
	}
}

bool MockServer::isRunning() const
{
	return m_tcpServer.isListening();
}

void MockServer::setupRoutes()
{
	m_httpServer.route("/version", []() {
		QJsonObject obj{
			{"version", "1.0.0"}
		};

		return QHttpServerResponse(
			"application/json",
			QJsonDocument(obj).toJson(QJsonDocument::Compact)

			);
	});

	// Plain text endpoint
	m_httpServer.route("/hello", []() {
		return QHttpServerResponse(
			"text/plain",
			"Hello from Qt HTTP Server\n",

			QHttpServerResponder::StatusCode::Ok
			);
	});

	// Plain text endpoint
	m_httpServer.route("/404", []() {
		QJsonObject obj{
			{"error", "Something does not exist"}
		};

		return QHttpServerResponse(
			"application/json",
			QJsonDocument(obj).toJson(QJsonDocument::Compact),
			QHttpServerResponder::StatusCode::NotFound

			);
	});
}
