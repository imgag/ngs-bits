#ifndef REQUESTWORKER_H
#define REQUESTWORKER_H


#include "cppREST_global.h"
#include <QThread>
#include <QDebug>
#include <QSslSocket>
#include <QSslError>
#include <QSslConfiguration>
#include <QList>

#include "Exceptions.h"
#include "RequestHandler.h"

class CPPRESTSHARED_EXPORT RequestWorker : public QThread
{
	Q_OBJECT
public:
	explicit RequestWorker(QSslConfiguration ssl_configuration, qintptr socket);
	void run();

protected slots:
	void handleConnection();

Q_SIGNALS:
	void sslFailed(const QList<QSslError> &error);
	void verificationFailed(const QSslError &error);
	void securelyConnected();


private:
	QString intToHex(const int &input);

	void closeAndDeleteSocket(QSslSocket* socket);
	void sendResponseChunk(QSslSocket *socket, QByteArray data);
	void sendEntireResponse(QSslSocket *socket, HttpResponse response);
	void finishChunckedResponse(QSslSocket* socket);

	QSslConfiguration ssl_configuration_;
	qintptr socket_;
};




#endif // REQUESTWORKER_H
