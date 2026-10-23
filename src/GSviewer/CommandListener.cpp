#include "CommandListener.h"

#include <QLocalSocket>
#include <QTcpSocket>

std::unique_ptr<CommandListener> CommandListener::fromString(const QString& addr, QObject* parent)
{
	bool ok = false;
	const quint16 port = addr.toUShort(&ok);

	if (ok)
	{
		return std::make_unique<TcpListener>(port, parent);
	}
	else
	{
		return std::make_unique<LocalListener>(addr, parent);
	}
}

CommandListener::CommandListener(const QString addr, QObject* parent)
	: QObject(parent)
	, address_(addr)
{
	connect(this, &CommandListener::closedConnection, this, [](QIODevice* socket)
	{
		socket->deleteLater();
	});
}

TcpListener::TcpListener(const quint16 port, QObject* parent)
	: CommandListener(QString::number(port), parent)
	, port_(port)
{
	connect(&sock_, &QTcpServer::newConnection, this, [this]
	{
		while (auto* s = sock_.nextPendingConnection())
		{
			connect(s, &QTcpSocket::disconnected, this, [this, s]
			{
				emit closedConnection(s);
				s->deleteLater();
			});
			emit newConnection(s);
		}
	});
}

void TcpListener::bind()
{
	if (!sock_.isListening()) sock_.listen(QHostAddress::LocalHost, port_);
}

void TcpListener::terminate()
{
	if (sock_.isListening()) sock_.close();

	const auto sockets = sock_.findChildren<QTcpSocket *>(Qt::FindDirectChildrenOnly);
	for (QTcpSocket *s : sockets) {
		s->abort();
		s->deleteLater();
	}
}

LocalListener::LocalListener(const QString& path, QObject* parent)
	: CommandListener(path, parent)
{
	sock_.removeServer(path);

	connect(&sock_, &QLocalServer::newConnection, this, [this]
	{
		while (auto* s = sock_.nextPendingConnection())
		{
			connect(s, &QLocalSocket::disconnected, this, [this, s]
			{
				emit closedConnection(s);
				s->deleteLater();
			});
			emit newConnection(s);
		}
	});
}

void LocalListener::bind()
{
	if (!sock_.isListening()) sock_.listen(address_);
}

void LocalListener::terminate()
{
	if (sock_.isListening()) sock_.close();

	const auto sockets = sock_.findChildren<QTcpSocket *>(Qt::FindDirectChildrenOnly);
	for (QTcpSocket *s : sockets) {
		s->abort();
		s->deleteLater();
	}
}
