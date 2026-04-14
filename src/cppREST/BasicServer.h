#ifndef BASICSERVER_H
#define BASICSERVER_H


#include <QObject>
#include <QTcpServer>
#include "cppREST_global.h"
#include <QtHttpServer/QHttpServer>
#include <QtHttpServer/QHttpServerResponse>

class CPPRESTSHARED_EXPORT BasicServer: public QObject
{
	Q_OBJECT

public:
	explicit BasicServer(quint16 port, QObject* parent = nullptr);

	bool isRunning() const;

	template<typename Handler>
	void addRoute(const QString &path, QHttpServerRequest::Method method, Handler &&handler)
	{
		http_server.route(path, method, std::forward<Handler>(handler));
	}

private:
	void setupDefaultRoutes();

	QHttpServer http_server;
	QTcpServer  tcp_server;
};

#endif // BASICSERVER_H
