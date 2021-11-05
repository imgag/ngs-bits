#include "SslServer.h"

SslServer::SslServer(QObject *parent, bool insecure) :
	QTcpServer(parent)
	, is_insecure_(insecure)
	, thread_count_(0)
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
	RequestWorker *request_worker;
	if (is_insecure_)
	{
		request_worker = new RequestWorker(socket);
	}
	else
	{
		request_worker = new RequestWorker(current_ssl_configuration_, socket);

	}
	connect(request_worker, &RequestWorker::finished, request_worker, &QObject::deleteLater);
	request_worker->start();
	thread_count_++;
	qDebug() << "NUMBER OF THE PROCESSED INCOMMING CONNECTIONS" << thread_count_;
}
