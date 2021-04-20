#include "HttpsServer.h"

HttpsServer::HttpsServer(quint16 port)
{
	QString ssl_certificate = ServerHelper::getStringSettingsValue("ssl_certificate");
	if (ssl_certificate.isEmpty())
	{
		ssl_certificate = QStandardPaths::writableLocation(QStandardPaths::HomeLocation) + QDir::separator() + "test-cert.crt";
		ServerHelper::debug("SSL certificate has not been specified in the config. Using a test certificate: " + ssl_certificate);
	}

	QFile certFile(ssl_certificate);
	if (!certFile.open(QIODevice::ReadOnly))
	{		
		ServerHelper::fatal("Unable to load SSL certificate");
        return;
    }

	QString ssl_key = ServerHelper::getStringSettingsValue("ssl_key");
	if (ssl_key.isEmpty())
	{
		ssl_key = QStandardPaths::writableLocation(QStandardPaths::HomeLocation) + QDir::separator() + "test-key.key";
		ServerHelper::debug("SSL key has not been specified in the config. Using a test key: " + ssl_key);
	}

	QFile keyFile(ssl_key);
	if (!keyFile.open(QIODevice::ReadOnly))
	{
		ServerHelper::fatal("Unable to load SSL key");
        return;
    }

    QSslCertificate cert(&certFile);
    QSslKey key(&keyFile, QSsl::Rsa);

	server_ = new SslServer(this);
//	connect(server_, SIGNAL(securelyConnected()), this, SLOT(handleConnection()));

	QSslConfiguration config = server_->getSslConfiguration();
    config.setLocalCertificate(cert);
    config.setPrivateKey(key);
	server_->setSslConfiguration(config);
	if (server_->listen(QHostAddress::Any, port))
	{		
		ServerHelper::info("HTTPS server is running on port #" + QString::number(port));

		QTimer *timer = new QTimer(this);
		connect(timer, &QTimer::timeout, this, &UrlManager::removeExpiredUrls);
		timer->start(10000); // every 10 seconds
	}
	else
	{		
		ServerHelper::critical("Could not start the HTTPS server on port #" + QString::number(port) + ":" + server_->serverError());
	}
}

void HttpsServer::handleConnection()
{
	while(server_->hasPendingConnections())
	{
		QSslSocket *sock = server_->nextPendingConnection();
		RequestHandler *handler = new RequestHandler(sock);
	}
}
