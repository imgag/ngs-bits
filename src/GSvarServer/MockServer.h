#ifndef MOCKSERVER_H
#define MOCKSERVER_H

#include <QObject>
#include <QTcpServer>


#include <QtHttpServer/QHttpServer>
#include <QtHttpServer/QHttpServerResponse>

class QHttpServer;

class MockServer : public QObject
{
	Q_OBJECT

public:
	explicit MockServer(quint16 port, QObject* parent = nullptr);
	bool isRunning() const;

private:
	void setupRoutes();

	QHttpServer m_httpServer;
	QTcpServer  m_tcpServer;
};




#endif
