#include "InsecureServer.h"

InsecureServer::InsecureServer(QObject* parent) :
	QTcpServer(parent)
{
//	threadPool = new QThreadPool(this);
//	threadPool->setMaxThreadCount(20); // max number of threads in our pool
}


void InsecureServer::incomingConnection(qintptr socket)
{
//	QSocketRunnable* runnable = new QSocketRunnable(handle);
//	threadPool->start(runnable);

	qInfo() << "HTTP incomming connection";
	InsecureRequestWorker *request_worker = new InsecureRequestWorker(socket);
	connect(request_worker, &InsecureRequestWorker::finished, request_worker, &QObject::deleteLater);
	request_worker->start();
}
