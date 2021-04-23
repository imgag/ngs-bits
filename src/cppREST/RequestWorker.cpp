#include "RequestWorker.h"
#include "HttpResponse.h"
#include "RequestParser.h"

RequestWorker::RequestWorker(QSslConfiguration ssl_configuration, qintptr socket)
	:
	 ssl_configuration_(ssl_configuration)
	, socket_(socket)
{
	qDebug() << "*** Initialize a new thread ***";
}

void RequestWorker::run()
{
	qDebug() << "*** Processing an incomming connection in a thread ***";
	QSslSocket *ssl_socket = new QSslSocket();

	if (!ssl_socket)
	{
		qCritical() << "Could not create a socket";
		return;
	}
	ssl_socket->setSslConfiguration(ssl_configuration_);

	if (!ssl_socket->setSocketDescriptor(socket_))
	{
		qCritical() << "Could not set a socket descriptor";
		delete ssl_socket;
		return;
	}

	typedef void (QSslSocket::* sslFailed)(const QList<QSslError> &);
	connect(ssl_socket, static_cast<sslFailed>(&QSslSocket::sslErrors), this, &RequestWorker::sslFailed);
	connect(ssl_socket, &QSslSocket::peerVerifyError, this, &RequestWorker::verificationFailed);
	connect(ssl_socket, &QSslSocket::encrypted, this, &RequestWorker::securelyConnected);
	connect(this, SIGNAL(securelyConnected()), this, SLOT(handleConnection()));

	ssl_socket->startServerEncryption();

	if (!ssl_socket->isOpen()) return;

	if (!ssl_socket->waitForReadyRead())
	{
		sendEntireResponse(ssl_socket, HttpResponse(HttpError{StatusCode::INTERNAL_SERVER_ERROR, ContentType::TEXT_HTML, "Request could not be processed"}));
		return;
	}
	else
	{
		if (!ssl_socket->isEncrypted()) closeAndDeleteSocket(ssl_socket);

		// Read the request
		QByteArray raw_request = ssl_socket->readAll();
		HttpRequest parsed_request;
		RequestPaser *parser = new RequestPaser(&raw_request, ssl_socket->peerAddress().toString());
		try
		{
			parsed_request = parser->getRequest();
		}
		catch (ArgumentException& e)
		{
			sendEntireResponse(ssl_socket, HttpResponse(HttpError{StatusCode::BAD_REQUEST, ContentType::TEXT_HTML, e.message()}));
			return;
		}

		// Process the request based on the endpoint info
		ServerHelper::info(parsed_request.methodAsString().toUpper() + "/" + parsed_request.getPath() + parsed_request.getRemoteAddress().toLatin1().data());

		Endpoint current_endpoint = EndpointManager::getEndpointEntity(parsed_request.getPath(), parsed_request.getMethod());
		if (current_endpoint.action_func == nullptr)
		{
			sendEntireResponse(ssl_socket, HttpResponse(HttpError{StatusCode::BAD_REQUEST, parsed_request.getContentType(), "This action cannot be processed"}));
			return;
		}

		try
		{
			EndpointManager::validateInputData(&current_endpoint, parsed_request);
		}
		catch (ArgumentException& e)
		{
			sendEntireResponse(ssl_socket, HttpResponse(HttpError{StatusCode::BAD_REQUEST, parsed_request.getContentType(), e.message()}));
			return;
		}

		ServerHelper::debug("Requested:" + current_endpoint.comment);
		HttpResponse (*endpoint_action_)(HttpRequest request) = current_endpoint.action_func;
		HttpResponse response;

		ServerHelper::debug("Trying to execute an action");
		try
		{
			response = (*endpoint_action_)(parsed_request);
		}
		catch (Exception& e)
		{
			ServerHelper::debug("Error while executing an action");
			sendEntireResponse(ssl_socket, HttpResponse(HttpError{StatusCode::INTERNAL_SERVER_ERROR, parsed_request.getContentType(), "Could not process endpoint action"}));
			return;
		}

		if (response.isStream())
		{
			ServerHelper::debug("Initiating a stream: " + response.getFilename());

			if (response.getFilename().isEmpty())
			{
				sendEntireResponse(ssl_socket, HttpResponse(HttpError{StatusCode::INTERNAL_SERVER_ERROR, parsed_request.getContentType(), "File name has not been found"}));
				return;
			}

			QFile streamed_file(response.getFilename());
			QFile::OpenMode mode = QFile::ReadOnly;

			if (!response.isBinary())
			{
				 mode = QFile::ReadOnly | QFile::Text;
			}

			try
			{
				streamed_file.open(mode);
			}
			catch (Exception& e)
			{
				ServerHelper::debug("Error while opening a file for streaming");
				sendEntireResponse(ssl_socket, HttpResponse(HttpError{StatusCode::INTERNAL_SERVER_ERROR, parsed_request.getContentType(), "Could not open a file for streaming: " + response.getFilename()}));
				return;
			}

			if (!streamed_file.isOpen())
			{
				ServerHelper::debug("File is not open");
				sendEntireResponse(ssl_socket, HttpResponse(HttpError{StatusCode::INTERNAL_SERVER_ERROR, parsed_request.getContentType(), "File is not open: " + response.getFilename()}));
				return;
			}

			sendResponseChunk(ssl_socket, response.getHeaders());
			if (response.isBinary())
			{
				ServerHelper::debug("Binary stream thread");
				qint64 chunk_size = 1024;
				qint64 pos = 0;

				ServerHelper::debug("Content type" + response.getHeaders());

				while(!streamed_file.atEnd())
				{
					streamed_file.seek(pos);
					QByteArray data = streamed_file.read(chunk_size);
					pos = pos + chunk_size;
					sendResponseChunk(ssl_socket, intToHex(data.size()).toLocal8Bit()+"\r\n");
					sendResponseChunk(ssl_socket, data.append("\r\n"));
				}
			}
			else
			{
				ServerHelper::debug("Text stream thread");
				QTextStream stream(&streamed_file);
				while(!stream.atEnd())
				{
					QByteArray line = stream.readLine().append("\n").toLocal8Bit();
					sendResponseChunk(ssl_socket, intToHex(line.size()).toLocal8Bit()+"\r\n");
					sendResponseChunk(ssl_socket, line+"\r\n");
				}
			}
			streamed_file.close();

			finishChunckedResponse(ssl_socket);
			return;
		}
		else if (!response.getPayload().isNull())
		{
			sendEntireResponse(ssl_socket, response);
			return;
		}

		sendEntireResponse(ssl_socket, HttpResponse(HttpError{StatusCode::NOT_FOUND, parsed_request.getContentType(), "This page does not exist. Check the URL and try again"}));
	}
}

void RequestWorker::handleConnection()
{
	ServerHelper::debug("Secure connection has been established");
}

QString RequestWorker::intToHex(const int& input)
{
	return QString("%1").arg(input, 10, 16, QLatin1Char('0')).toUpper();
}

void RequestWorker::closeAndDeleteSocket(QSslSocket* socket)
{
	socket->flush();
	socket->close();
	socket->deleteLater();
}

void RequestWorker::sendResponseChunk(QSslSocket* socket, QByteArray data)
{
	// clinet cancels the stream or simply disconnects
	if ((!socket->isValid()) || (socket->state() == QSslSocket::SocketState::UnconnectedState))
	{
		ServerHelper::debug("Socket is disconnected and no longer used");
		this->terminate();
		return;
	}

	if (socket->bytesToWrite())
	{
		socket->flush();
		socket->waitForBytesWritten();
	}

	socket->write(data);
}

void RequestWorker::sendEntireResponse(QSslSocket* socket, HttpResponse response)
{
	ServerHelper::debug("Writing an entire response");
	socket->write(response.getHeaders());
	socket->write(response.getPayload());
	closeAndDeleteSocket(socket);
}

void RequestWorker::finishChunckedResponse(QSslSocket* socket)
{
	if (socket->bytesToWrite())
	{
		socket->flush();
		socket->waitForBytesWritten();
	}

	if (!socket->bytesToWrite())
	{
		ServerHelper::debug("Closing the socket");
		socket->write("0\r\n");
		socket->write("\r\n");
		closeAndDeleteSocket(socket);
		return;
	}

	ServerHelper::debug("Closing the socket forcefully");
	closeAndDeleteSocket(socket);
}
