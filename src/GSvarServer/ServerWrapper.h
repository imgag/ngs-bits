#ifndef SERVERWRAPPER_H
#define SERVERWRAPPER_H

#include <QObject>
#include <QSslKey>
#include <QCoreApplication>
#include <QTimer>
#include <QFileSystemWatcher>
#include "Log.h"
#include "SslServer.h"
#include "ClientHelper.h"

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
	void updateQueingEngineStatus();

private:
	ClientInfo readClientInfoFromFile();
	QByteArray readUserNotificationFromFile();
	QSslKey readPrivateKey(const QString &filePath, const QByteArray &passPhrase = QByteArray());
	QSharedPointer<SslServer> server_;
	bool is_running_;

	QThreadPool background_task_pool_;
	QThreadPool cleanup_pool_;
	QThreadPool qe_status_pool_;

	QTimer session_timer_;
	QTimer qe_status_update_timer_;
	QTimer clinvar_timer_;
	QTimer log_check_timer_;

	QFileSystemWatcher watcher_;
};

#endif // SERVERWRAPPER_H
