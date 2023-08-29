#include "SslServer.h"

SslServer::SslServer(QObject *parent)
    : QTcpServer(parent)
    , thread_pool_()
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
    try
    {
        RequestWorker *request_worker = new RequestWorker(current_ssl_configuration_, socket);
        thread_pool_.start(request_worker);
    }
    catch (...)
    {
        Log::error("Unexpected error while processing a client request");
    }
}
