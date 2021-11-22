#include "ServerWrapper.h"

ServerWrapper::ServerWrapper(const quint16& port, const bool& insecure)
{
	QString protocol_name;
	if (!insecure)
	{
		protocol_name = "HTTPS";
		QString ssl_certificate = ServerHelper::getStringSettingsValue("ssl_certificate");
		if (ssl_certificate.isEmpty())
		{
			ssl_certificate = QStandardPaths::writableLocation(QStandardPaths::HomeLocation) + QDir::separator() + "test-cert.crt";
			qDebug() << "SSL certificate has not been specified in the config. Using a test certificate: " + ssl_certificate;
		}

		QFile certFile(ssl_certificate);
		if (!certFile.open(QIODevice::ReadOnly))
		{
			qFatal("Unable to load SSL certificate");
			return;
		}

		QString ssl_key = ServerHelper::getStringSettingsValue("ssl_key");
		if (ssl_key.isEmpty())
		{
			ssl_key = QStandardPaths::writableLocation(QStandardPaths::HomeLocation) + QDir::separator() + "test-key.key";
			qDebug() << "SSL key has not been specified in the config. Using a test key: " + ssl_key;
		}

		QFile keyFile(ssl_key);
		if (!keyFile.open(QIODevice::ReadOnly))
		{
			qFatal("Unable to load SSL key");
			return;
		}

		QString ssl_chain = ServerHelper::getStringSettingsValue("ssl_certificate_chain");
		QList<QSslCertificate> ca_certificates;
		if (!ssl_chain.isEmpty())
		{
			qInfo() << "Reading SSL certificate chain file";
			ca_certificates = QSslCertificate::fromPath(ssl_chain, QSsl::Pem);
		}

		QSslCertificate cert(&certFile);
		QSslKey key(&keyFile, QSsl::Rsa);

		server_ = new SslServer(this);

		QSslConfiguration config = server_->getSslConfiguration();
		config.setLocalCertificate(cert);
		config.setPrivateKey(key);

		if (ca_certificates.size()>0)
		{
			qInfo() << "Loading SSL certificate chain";
			config.setLocalCertificateChain(ca_certificates);
		}

		server_->setSslConfiguration(config);
	}
	else
	{
		protocol_name = "HTTP";
		qInfo() << "Insecure server option has been selected";
		server_ = new SslServer(this, true);
	}

	if (server_->listen(QHostAddress::Any, port))
	{
		qInfo() << protocol_name << "server is running on port #" + QString::number(port);
		QTimer *timer = new QTimer(this);
		connect(timer, &QTimer::timeout, this, &UrlManager::removeExpiredUrls);
		timer->start(10000); // every 10 seconds
	}
	else
	{
		qCritical() << "Could not start " << protocol_name << " server on port #" + QString::number(port) + ":" + server_->serverError();
	}

}
