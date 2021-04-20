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

//signals:
//	void dataChunkReady(QSslSocket *socket, const QByteArray &data);
//	void allDataReady(QSslSocket *socket, const HttpResponse &response);

private:
	QString intToHex(const int &input);

	void dataChunkReady(QSslSocket *socket, QByteArray data);
	void allDataReady(QSslSocket *socket, HttpResponse response);

	QSslConfiguration ssl_configuration_;
	qintptr socket_;
};




#endif // REQUESTWORKER_H
