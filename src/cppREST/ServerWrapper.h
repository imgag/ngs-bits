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
	ServerWrapper(const quint16& port, const bool& insecure = false);

private:
	SslServer *server_;
};

#endif // SERVERWRAPPER_H
