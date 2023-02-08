#include "ServerWrapper.h"

ServerWrapper::ServerWrapper(const quint16& port)
	: is_running_(false)
{	
	QString ssl_certificate = ServerHelper::getStringSettingsValue("ssl_certificate");
	if (ssl_certificate.isEmpty())
	{
		ssl_certificate = QStandardPaths::writableLocation(QStandardPaths::HomeLocation) + QDir::separator() + "test-cert.crt";
		Log::warn("SSL certificate has not been specified in the config. Using a test certificate: " + ssl_certificate);
	}

	QFile certFile(ssl_certificate);
	if (!certFile.open(QIODevice::ReadOnly))
	{
		Log::error("Unable to load SSL certificate");
		return;
	}

	QString ssl_key = ServerHelper::getStringSettingsValue("ssl_key");
	if (ssl_key.isEmpty())
	{
		ssl_key = QStandardPaths::writableLocation(QStandardPaths::HomeLocation) + QDir::separator() + "test-key.key";
		Log::warn("SSL key has not been specified in the config. Using a test key: " + ssl_key);
	}

	QFile keyFile(ssl_key);
	if (!keyFile.open(QIODevice::ReadOnly))
	{
		Log::error("Unable to load SSL key");
		return;
	}

	QString ssl_chain = ServerHelper::getStringSettingsValue("ssl_certificate_chain");
	QList<QSslCertificate> ca_certificates;
	if (!ssl_chain.isEmpty())
	{
		Log::info("Reading SSL certificate chain file");
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
		Log::info("Loading SSL certificate chain");
		config.setLocalCertificateChain(ca_certificates);
	}
	server_->setSslConfiguration(config);


	if (server_->listen(QHostAddress::Any, port))
	{
		is_running_ = true;
		Log::info("GSvar server is running on port #" + QString::number(port));

		// Remove expired URLs on schedule
		QTimer *url_timer = new QTimer(this);
		connect(url_timer, &QTimer::timeout, this, &UrlManager::removeExpiredUrls);
		url_timer->start(60 * 30 * 1000); // every 30 minutes

		// Remove expired sessions (invalidate tokens) on schedule
		QTimer *session_timer = new QTimer(this);
		connect(session_timer, &QTimer::timeout, this, &SessionManager::removeExpiredSessions);
		url_timer->start(60 * 30 * 1000); // every 30 minutes
	}
	else
	{		
		Log::error("Could not start GSvar server on port #" + QString::number(port) + ": " + server_->errorString());
	}

}

bool ServerWrapper::isRunning() const
{
	return is_running_;
}
