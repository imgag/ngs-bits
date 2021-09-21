#ifndef SSLREQUESTWORKER_H
#define SSLREQUESTWORKER_H

#include "cppREST_global.h"
#include <QThread>
#include <QDebug>
#include <QSslSocket>
#include <QSslError>
#include <QSslConfiguration>
#include <QHostAddress>
#include <QList>

#include "Exceptions.h"
#include "HttpResponse.h"
#include "RequestParser.h"
#include "EndpointManager.h"

class CPPRESTSHARED_EXPORT SslRequestWorker : public QThread
{
	Q_OBJECT
public:
	SslRequestWorker(QSslConfiguration ssl_configuration, qintptr socket);
	void run();

protected slots:
	void handleConnection();
	void socketDisconnected();

Q_SIGNALS:
	void sslFailed(const QList<QSslError> &error);
	void verificationFailed(const QSslError &error);
	void securelyConnected();


private:
	QString intToHex(const int &input);

	void closeAndDeleteSocket(QSslSocket* socket);
	void sendResponseDataPart(QSslSocket *socket, QByteArray data);
	void sendEntireResponse(QSslSocket *socket, HttpResponse response);
	void finishPartialDataResponse(QSslSocket* socket);

	QSslConfiguration ssl_configuration_;
	qintptr socket_;
	bool is_terminated_;
};

#endif // SSLREQUESTWORKER_H
