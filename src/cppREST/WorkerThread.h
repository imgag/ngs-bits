#ifndef WORKERTHREAD_H
#define WORKERTHREAD_H

#include "cppREST_global.h"
#include <QThread>
#include <QFile>
#include <QDebug>
#include <QDir>
#include "HttpRequest.h"
#include "EndpointManager.h"

class CPPRESTSHARED_EXPORT WorkerThread : public QThread
{
	Q_OBJECT
public:
	explicit WorkerThread(HttpRequest request);
	void run();

private:	
	HttpResponse (*endpoint_action_)(HttpRequest request);
	HttpRequest request_;
	QString intToHex(const int &in);

signals:
	void dataChunkReady(const QByteArray &data);
	void resultReady(const HttpResponse &response);
};

#endif // WORKERTHREAD_H
