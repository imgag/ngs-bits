#ifndef SSLSERVER_P_H
#define SSLSERVER_P_H

#include "cppREST_global.h"
#include <QTcpServer>
#include <QSslSocket>
#include <QSslError>
#include <QSslConfiguration>
#include <QList>
#include "Exceptions.h"
#include "RequestWorker.h"

class CPPRESTSHARED_EXPORT SslServer : public QTcpServer
{
    Q_OBJECT

public:
	SslServer(QObject *parent = nullptr, bool insecure = false);
	virtual ~SslServer();
	QSslConfiguration getSslConfiguration() const;
	void setSslConfiguration(const QSslConfiguration &ssl_configuration);
    QSslSocket *nextPendingConnection();

Q_SIGNALS:	
	void sslFailed(const QList<QSslError> &error);
	void verificationFailed(const QSslError &error);
	void securelyConnected();

protected:
    virtual void incomingConnection(qintptr socket);

private:
	QSslConfiguration current_ssl_configuration_;
	bool is_insecure_;
};

#endif // SSLSERVER_P_H
