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

		QFileSystemWatcher *watcher = new QFileSystemWatcher();
		watcher->addPath(QCoreApplication::applicationDirPath());
		connect(watcher, SIGNAL(directoryChanged(QString)), this, SLOT(updateInfoForUsers(QString)));

		// Read the client version and notification information during the initialization
		SessionManager::setCurrentClientInfo(readClientInfoFromFile());
		SessionManager::setCurrentNotification(readUserNotificationFromFile());
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

void ServerWrapper::updateInfoForUsers(QString str)
{
	QDir dir;
	dir.setPath(str);
	if (!dir.exists()) return;
	QFileInfoList list = dir.entryInfoList(QDir::NoDotAndDotDot | QDir::Files, QDir::Time);
	if (list.size() == 0) return;

	if (list.first().fileName() == CLIENT_INFO_FILE)
	{
		SessionManager::setCurrentClientInfo(readClientInfoFromFile());
		Log::info("Updated the desktop client version information to: " + SessionManager::getCurrentClientInfo().version);
	}

	if (list.first().fileName() == NOTIFICATION_FILE)
	{
		SessionManager::setCurrentNotification(readUserNotificationFromFile());
		Log::info("A new notification for the users has been providied");
	}
}

ClientInfo ServerWrapper::readClientInfoFromFile()
{

	if (!QFile(QCoreApplication::applicationDirPath() + QDir::separator() + CLIENT_INFO_FILE).exists())
	{
		QFile empty_file(QCoreApplication::applicationDirPath() + QDir::separator() + CLIENT_INFO_FILE);
		if (!empty_file.open(QIODevice::WriteOnly)) Log::error("Could not create the client info file");
	}

	ClientInfo info;
	try
	{
		QSharedPointer<QFile> client_info_file = Helper::openFileForReading(QCoreApplication::applicationDirPath() + QDir::separator() + CLIENT_INFO_FILE, false);
		QByteArray content;
		while(!client_info_file->atEnd())
		{
			QString line = client_info_file->readLine().trimmed();
			content.append(line);
		}
		QJsonDocument json_input = QJsonDocument::fromJson(content);

		if (json_input.isObject())
		{
			if (json_input.object().contains("version")) info.version = json_input.object().value("version").toString();
			if (json_input.object().contains("message")) info.message = json_input.object().value("message").toString();
			if (json_input.object().contains("date")) info.date = QDateTime::fromString(json_input.object().value("date").toString());
		}
	}
	catch (Exception& e)
	{
		Log::error("Error while reading client version information: " + e.message());
	}
	Log::info("Reading the client version information from the settings: " + info.version);
	if (info.version.isEmpty()) Log::warn("Client version information file is empty");
	return info;
}

QByteArray ServerWrapper::readUserNotificationFromFile()
{
	if (!QFile(QCoreApplication::applicationDirPath() + QDir::separator() + NOTIFICATION_FILE).exists())
	{
		QFile empty_file(QCoreApplication::applicationDirPath() + QDir::separator() + NOTIFICATION_FILE);
		if (!empty_file.open(QIODevice::WriteOnly)) Log::error("Could not create the notification file");
	}

	QByteArray content;
	try
	{
		QSharedPointer<QFile> client_info_file = Helper::openFileForReading(QCoreApplication::applicationDirPath() + QDir::separator() + NOTIFICATION_FILE, false);
		while(!client_info_file->atEnd())
		{
			QString line = client_info_file->readLine().trimmed();
			content.append(line);
		}
	}
	catch (Exception& e)
	{
		Log::error("Error while reading the notification file: " + e.message());
	}

	return content;
}
