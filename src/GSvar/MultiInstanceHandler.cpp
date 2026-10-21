#include "MultiInstanceHandler.h"

#include <QLocalSocket>
#include <QDataStream>
#include <QIODevice>
#include "Log.h"

MultiInstanceHandler::MultiInstanceHandler(QObject* parent)
	: QObject(parent)
	, server_(QSharedPointer<QLocalServer>::create())
{
	connect(server_.data(), &QLocalServer::newConnection,
	this, [this]()
	{
		while (server_->hasPendingConnections())
		{
			QLocalSocket* socket = server_->nextPendingConnection();
			if (socket == nullptr) continue;

			// The socket is owned by the server's pending-connection
			// mechanism until retrieved. After retrieval, ensure it has
			// a parent that will clean it up with this handler
			socket->setParent(this);

			connect(socket, &QLocalSocket::readyRead,
			this, [this, socket]()
			{
				while (socket->canReadLine())
				{
					const QByteArray request = socket->readLine().trimmed();
					const QUrl url(QString::fromUtf8(request));

					if (!url.isValid() || url.scheme() != "gsvar")
					{
						socket->write("ERROR\n");
						socket->flush();
						continue;
					}

					// Acknowledge receipt before sending the URL
					if (socket->write("OK\n") < 0)
					{
						Log::error("Failed to write socket acknowledgement");
						return;
					}

					socket->flush();

					// Process the URL after this callback finishes
					QMetaObject::invokeMethod(
					this,
					[this, url]()
					{
						emit urlReceived(url);
					},
					Qt::QueuedConnection);
				}
			});

			connect(socket, &QLocalSocket::disconnected, socket, &QObject::deleteLater);
		}
	});
}

bool MultiInstanceHandler::anotherInstanceExists()
{
	QLocalSocket socket;
	socket.connectToServer(SERVER_NAME);
	if (!socket.waitForConnected(1000)) return false;

	return true;
}

bool MultiInstanceHandler::startServer()
{
	if (!server_->listen(SERVER_NAME))
	{
		server_->removeServer(SERVER_NAME);
		return server_->listen(SERVER_NAME);
	}	
	return false;
}

QString MultiInstanceHandler::errorString()
{
	return server_->errorString();
}

bool MultiInstanceHandler::forwardToExistingInstance(const QUrl& url)
{
	QLocalSocket socket;

	socket.connectToServer(SERVER_NAME);

	if (!socket.waitForConnected(1000))
	{
		Log::error("Could not connect to another instance");
		return false;
	}

	// Send the fully encoded URL as a newline-terminated message
	QByteArray request = url.toString(QUrl::FullyEncoded).toUtf8();
	request.append('\n');

	if (socket.write(request) != request.size())
	{
		Log::error("Failed to send the entire request to another instance");
		return false;
	}

	if (!socket.waitForBytesWritten(1000))
	{
		Log::error("Failed to forward the URL to another instance");
		return false;
	}

	// Wait for the application-level acknowledgement
	QByteArray response;
	while (!response.contains('\n'))
	{
		if (!socket.waitForReadyRead(3000))
		{
			Log::error("Failed to get the confirmation from another instance");
			return false;
		}
		response.append(socket.readAll());
	}
	const QByteArray acknowledgement = response.left(response.indexOf('\n')).trimmed();

	return acknowledgement == "OK";
}