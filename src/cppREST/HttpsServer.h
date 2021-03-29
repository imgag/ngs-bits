#ifndef HTTPSSERVER_H
#define HTTPSSERVER_H

#include "cppREST_global.h"
#include <QObject>
#include <QDebug>
#include <QFile>
#include <QSslCertificate>
#include <QSslKey>
#include <QSslConfiguration>
#include <QSslSocket>
#include <QStandardPaths>
#include <QTimer>

#include "SslServer.h"
#include "RequestHandler.h"
#include "UrlManager.h"

class CPPRESTSHARED_EXPORT HttpsServer : public QObject
{
    Q_OBJECT

public:
	HttpsServer(quint16 port);   

protected slots:
    void handleConnection();	

private:
	SslServer *server_;
};

#endif // HTTPSSERVER_H
