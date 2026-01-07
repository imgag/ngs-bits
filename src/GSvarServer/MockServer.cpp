#include "MockServer.h"
#include "Log.h"
#include <QHttpServer>

MockServer::MockServer(quint16 port, QObject* parent)
	: QObject(parent)
{
	httpServer_->route("/", []() {
		Log::info("Route hit");
		return QByteArray("hello world\n");
	});

	if (!tcpServer_.listen(QHostAddress::Any, port)) {
		Log::info("Failed to listen on port");
		return;
	}

	if (!httpServer_->bind(&tcpServer_)) {
		Log::info("Failed to bind HTTP server");
		return;
	}

	Log::info("Listening on port " + QString::number(tcpServer_.serverPort()));
}

bool MockServer::is_running() const
{
	return tcpServer_.isListening();
}
