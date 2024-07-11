#ifndef SERVERWRAPPER_H
#define SERVERWRAPPER_H

#include <QObject>
#include <QFile>
#include <QSslCertificate>
#include <QSslKey>
#include <QSslConfiguration>
#include <QSslSocket>
#include <QStandardPaths>
#include <QTimer>
#include <QFileSystemWatcher>

#include "Log.h"
#include "SslServer.h"
#include "UrlManager.h"

class ServerWrapper : public QObject
{
    Q_OBJECT

public:
	const QString CLIENT_INFO_FILE = QCoreApplication::applicationName().replace(".exe", "") + "_client.json";
	const QString NOTIFICATION_FILE = QCoreApplication::applicationName().replace(".exe", "") + "_notification.json";
	ServerWrapper(const quint16& port);
	bool isRunning() const;

public slots:
    void updateClinVarSubmissionStatus();
	void updateInfoForUsers(QString str);
    void switchLogFile();
    void cleanupSessionsAndUrls();
    void updateSgeStatus();

private:
	ClientInfo readClientInfoFromFile();
	QByteArray readUserNotificationFromFile();
    QSslKey readPrivateKey(const QString &filePath, const QByteArray &passPhrase = QByteArray());
	SslServer *server_;
    bool is_running_;
    QThreadPool cleanup_pool_;
    QThreadPool sge_status_pool_;
};

#endif // SERVERWRAPPER_H
