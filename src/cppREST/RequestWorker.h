#ifndef REQUESTWORKER_H
#define REQUESTWORKER_H

#include "cppREST_global.h"
#include <QRunnable>
#include <QSslSocket>
#include <QSslConfiguration>
#include "ServerHelper.h"
#include "HttpResponse.h"

class CPPRESTSHARED_EXPORT RequestWorker
	: public QRunnable
{

public:
	explicit RequestWorker(QSslConfiguration ssl_configuration, qintptr socket, RequestWorkerParams params);
	void run() override;

private:
	const int STREAM_CHUNK_SIZE = 1024*10;
	QString intToHex(int input);

	void closeConnection(QSharedPointer<QSslSocket> socket);
	void sendResponseDataPart(QSharedPointer<QSslSocket> socket, const QByteArray& data);
	void sendEntireResponse(QSharedPointer<QSslSocket>, const HttpResponse& response);

	QSslConfiguration ssl_configuration_;
	qintptr socket_;
	RequestWorkerParams params_;
	bool is_terminated_;
};

#endif // REQUESTWORKER_H
