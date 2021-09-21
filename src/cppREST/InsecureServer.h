#ifndef INSECURESERVER_H
#define INSECURESERVER_H

#include <QTcpServer>
#include <QThreadPool>
#include "InsecureRequestWorker.h"

class InsecureServer : public QTcpServer
{
	Q_OBJECT
public:
	explicit InsecureServer(QObject *parent = 0);
	void incomingConnection(qintptr socket);

signals:

public slots:

private:
	QThreadPool* threadPool;
};

#endif // INSECURESERVER_H
