#include "ServerWrapper.h"
#include "UrlBackupWorker.h"
#include "SessionBackupWorker.h"

ServerWrapper::ServerWrapper(const quint16& port)
	: is_running_(false)
    , cleanup_pool_()
{
    cleanup_pool_.setMaxThreadCount(2);
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

    try
    {
        if (server_->listen(QHostAddress::Any, port))
        {
            is_running_ = true;
            Log::info("GSvar server is running on port #" + QString::number(port));

            // Remove expired URLs on schedule
            QTimer *url_timer = new QTimer(this);
            connect(url_timer, SIGNAL(timeout()), this, SLOT(cleanupUrls()));
            url_timer->start(60 * 5 * 1000); // every 5 minutes

            // Remove expired sessions (invalidate tokens) on schedule
            QTimer *session_timer = new QTimer(this);
            connect(session_timer, SIGNAL(timeout()), this, SLOT(cleanupSessions()));
            session_timer->start(60 * 10 * 1000); // every 10 minutes

            // ClinVar submission status automatic update on schedule
            QTimer *clinvar_timer = new QTimer(this);
            connect(clinvar_timer, SIGNAL(timeout()), this, SLOT(updateClinVarSubmissionStatus()));
            clinvar_timer->start(60 * 60 * 1000); // every 60 minutes

            // Switch to a new log file on schedule (to avoid using large files)
            QTimer *log_check_timer = new QTimer(this);
            connect(log_check_timer, SIGNAL(timeout()), this, SLOT(switchLogFile()));
            log_check_timer->start(60 * 60 * 1000); // every 60 minutes

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
    catch (Exception& e)
    {
        Log::error("Failed to start the server: " + e.message());
    }
}

bool ServerWrapper::isRunning() const
{
    return is_running_;
}

void ServerWrapper::updateClinVarSubmissionStatus()
{
    try
    {
        QPair<int,int> var_counts = NGSD().updateClinvarSubmissionStatus(false);
        Log::info("The submission status of " + QString::number(var_counts.first) + " published varaints has been checked, " + QString::number(var_counts.second) + " NGSD entries were updated." );
    }
    catch (Exception& e)
    {
        Log::error("An error has been detected while updating a ClinVar submission status: " + e.message());
    }
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

void ServerWrapper::switchLogFile()
{
    QString new_log_file = ServerHelper::getCurrentServerLogFile();
    if (Log::fileName()!=new_log_file)
    {
        Log::setFileName(new_log_file);
        Log::info("Started a new log file: " + new_log_file);
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
            content.append(line.toUtf8());
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
		QSharedPointer<QFile> notification_file = Helper::openFileForReading(QCoreApplication::applicationDirPath() + QDir::separator() + NOTIFICATION_FILE, false);
		while(!notification_file->atEnd())
		{
			QString line = notification_file->readLine().trimmed();
            content.append(line.toUtf8());
		}
	}
	catch (Exception& e)
	{
		Log::error("Error while reading the notification file: " + e.message());
	}

    return content;
}

void ServerWrapper::cleanupUrls()
{
    Log::info("Backup URLs on timer");
    try
    {
        UrlBackupWorker *url_backup_worker = new UrlBackupWorker(UrlManager::removeExpiredUrls());
        cleanup_pool_.start(url_backup_worker);
    }
    catch (FileAccessException& e)
    {
        Log::error("Error while accessing a URL backup file: " + e.message());
    }
    catch (...)
    {
         Log::error("Unexpected error while trying to backup URLs");
    }
}

void ServerWrapper::cleanupSessions()
{
    Log::info("Backup sessions on timer");
    try
    {
        SessionBackupWorker *session_backup_worker = new SessionBackupWorker(SessionManager::removeExpiredSessions());
        cleanup_pool_.start(session_backup_worker);
    }
    catch (FileAccessException& e)
    {
        Log::error("Error while accessing a session backup file: " + e.message());
    }
    catch (...)
    {
        Log::error("Unexpected error while trying to backup sessions");
    }
}
