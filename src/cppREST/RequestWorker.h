#ifndef REQUESTWORKER_H
#define REQUESTWORKER_H

#include "cppREST_global.h"
#include <QThread>
#include <QSslSocket>
#include <QSslError>
#include <QSslConfiguration>
#include <QHostAddress>
#include <QList>

#include "Log.h"
#include "Exceptions.h"
#include "HttpResponse.h"
#include "RequestParser.h"
#include "EndpointManager.h"

class CPPRESTSHARED_EXPORT RequestWorker : public QThread
{
	Q_OBJECT
public:
	RequestWorker(QSslConfiguration ssl_configuration, qintptr socket);
	RequestWorker(qintptr socket);
	void run();

protected slots:
	void handleConnection();
	void socketDisconnected();

Q_SIGNALS:
	void sslFailed(const QList<QSslError> &error);
	void verificationFailed(const QSslError &error);
	void securelyConnected();


private:
	const int STREAM_CHUNK_SIZE = 1024*10;
	QString intToHex(const int &input);

	void closeAndDeleteSocket(QSslSocket* socket);
	void sendResponseDataPart(QSslSocket *socket, QByteArray data);
	void sendEntireResponse(QSslSocket *socket, HttpResponse response);

	QSslConfiguration ssl_configuration_;
	qintptr socket_;
	bool is_terminated_;
	bool is_secure_;
};

#endif // REQUESTWORKER_H
