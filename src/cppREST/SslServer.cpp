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
	QSslSocket *ssl_socket = new QSslSocket();

	if (!ssl_socket)
	{
		THROW(Exception, "Could not create a socket");
		return;
	}
	ssl_socket->setSslConfiguration(current_ssl_configuration_);

	if (!ssl_socket->setSocketDescriptor(socket))
	{
		THROW(Exception, "Could not set a socket descriptor");
		delete ssl_socket;
		return;
	}

	typedef void (QSslSocket::* sslFailed)(const QList<QSslError> &);
	connect(ssl_socket, static_cast<sslFailed>(&QSslSocket::sslErrors), this, &SslServer::sslFailed);
	connect(ssl_socket, &QSslSocket::peerVerifyError, this, &SslServer::verificationFailed);
	connect(ssl_socket, &QSslSocket::encrypted, this, &SslServer::securelyConnected);
	addPendingConnection(ssl_socket);
	ssl_socket->startServerEncryption();
}
