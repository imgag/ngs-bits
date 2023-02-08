#ifndef SERVERWRAPPER_H
#define SERVERWRAPPER_H

#include "cppREST_global.h"
#include <QObject>
#include <QFile>
#include <QSslCertificate>
#include <QSslKey>
#include <QSslConfiguration>
#include <QSslSocket>
#include <QStandardPaths>
#include <QTimer>

#include "Log.h"
#include "SslServer.h"
#include "UrlManager.h"

class CPPRESTSHARED_EXPORT ServerWrapper : public QObject
{
    Q_OBJECT

public:
	ServerWrapper(const quint16& port);
	bool isRunning() const;

private:
	SslServer *server_;
	bool is_running_;
};

#endif // SERVERWRAPPER_H
