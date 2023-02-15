#include "SslServer.h"

SslServer::SslServer(QObject *parent) :
	QTcpServer(parent)
{
	current_ssl_configuration_ = QSslConfiguration::defaultConfiguration();
}

SslServer::~SslServer()
{
}

QSslConfiguration SslServer::getSslConfiguration() const
{
	return current_ssl_configuration_;
}

void SslServer::setSslConfiguration(const QSslConfiguration &ssl_configuration)
{
	current_ssl_configuration_ = ssl_configuration;
}

QSslSocket *SslServer::nextPendingConnection()
{
    return static_cast<QSslSocket *>(QTcpServer::nextPendingConnection());
}

void SslServer::incomingConnection(qintptr socket)
{
	RequestWorker *request_worker = new RequestWorker(current_ssl_configuration_, socket);
	connect(request_worker, &RequestWorker::finished, request_worker, &QObject::deleteLater);
	request_worker->start();
}
