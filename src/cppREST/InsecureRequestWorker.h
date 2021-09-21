#ifndef INSECUREREQUESTWORKER_H
#define INSECUREREQUESTWORKER_H

#include "cppREST_global.h"
#include <QThread>
#include <QDebug>
#include <QHostAddress>
#include <QList>

#include "Exceptions.h"
#include "HttpResponse.h"
#include "RequestParser.h"
#include "EndpointManager.h"

class CPPRESTSHARED_EXPORT InsecureRequestWorker : public QThread
{
	Q_OBJECT
public:
	InsecureRequestWorker(qintptr socket);
	void run();

protected slots:
	void handleConnection();
	void socketDisconnected();

private:
	QString intToHex(const int &input);

	void closeAndDeleteSocket(QTcpSocket* socket);
	void sendResponseDataPart(QTcpSocket *socket, QByteArray data);
	void sendEntireResponse(QTcpSocket *socket, HttpResponse response);
	void finishPartialDataResponse(QTcpSocket* socket);

	qintptr socket_;
	bool is_terminated_;
};

#endif // INSECUREREQUESTWORKER_H
