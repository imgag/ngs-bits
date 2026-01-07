#ifndef MOCKSERVER_H
#define MOCKSERVER_H

#include <QObject>
#include <QTcpServer>


class QHttpServer;

class MockServer : public QObject
{
	Q_OBJECT

public:
	explicit MockServer(quint16 port, QObject* parent = nullptr);
	bool is_running() const;

private:
	QHttpServer* httpServer_;
	QTcpServer tcpServer_;
};

#endif
