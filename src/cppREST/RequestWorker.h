#ifndef REQUESTWORKER_H
#define REQUESTWORKER_H

#include "cppREST_global.h"
#include <QRunnable>
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

class CPPRESTSHARED_EXPORT RequestWorker
    : public QRunnable
{

public:
    explicit RequestWorker(QSslConfiguration ssl_configuration, qintptr socket);
    void run() override;

private:
	const int STREAM_CHUNK_SIZE = 1024*10;
	QString intToHex(const int &input);

	void closeConnection(QSslSocket* socket);
	void sendResponseDataPart(QSslSocket *socket, QByteArray data);
	void sendEntireResponse(QSslSocket *socket, HttpResponse response);

	QSslConfiguration ssl_configuration_;
	qintptr socket_;
	bool is_terminated_;
};

#endif // REQUESTWORKER_H
