#include "RequestWorker.h"

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
	qDebug() << "Starting the encryption";
	ssl_socket->startServerEncryption();

	if (!ssl_socket->isOpen()) return;
	qDebug() << "Wait for the socket to be ready";
	if (!ssl_socket->waitForReadyRead())
	{
		if (ssl_socket->isOpen())
		{
			sendEntireResponse(ssl_socket, HttpResponse(HttpError{StatusCode::INTERNAL_SERVER_ERROR, ContentType::TEXT_HTML, "Request could not be processed"}));
		}
		qDebug() << "Socket is not ready. Exiting";
		return;
	}
	else
	{
		qDebug() << "Start the processing";
		if (!ssl_socket->isEncrypted()) closeAndDeleteSocket(ssl_socket);
		qDebug() << "Read the request";
		// Read the request
		QByteArray raw_request = ssl_socket->readAll();

		qDebug() << "Raw request" << raw_request;
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
		qInfo() << parsed_request.methodAsString().toUpper() + "/" + parsed_request.getPath() + parsed_request.getRemoteAddress().toLatin1().data();

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
			qDebug() << "Validation has failed";
			sendEntireResponse(ssl_socket, HttpResponse(HttpError{StatusCode::BAD_REQUEST, parsed_request.getContentType(), e.message()}));
			return;
		}

		qDebug() << "Requested:" + current_endpoint.comment;
		HttpResponse (*endpoint_action_)(HttpRequest request) = current_endpoint.action_func;
		HttpResponse response;

		qDebug() << "Trying to execute an action";
		qDebug() << "Headers = " << parsed_request.getHeaders();
		qDebug() << "vars = " << parsed_request.getPathParams();
		try
		{
			response = (*endpoint_action_)(parsed_request);
		}
		catch (Exception& e)
		{
			qDebug() << "Error while executing an action";
			sendEntireResponse(ssl_socket, HttpResponse(HttpError{StatusCode::INTERNAL_SERVER_ERROR, parsed_request.getContentType(), "Could not process endpoint action"}));
			return;
		}

		if (response.isStream())
		{
			qDebug() << "Initiating a stream: " + response.getFilename();

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
				qDebug() << "Error while opening a file for streaming";
				sendEntireResponse(ssl_socket, HttpResponse(HttpError{StatusCode::INTERNAL_SERVER_ERROR, parsed_request.getContentType(), "Could not open a file for streaming: " + response.getFilename()}));
				return;
			}

			if (!streamed_file.isOpen())
			{
				qDebug() << "File is not open";
				sendEntireResponse(ssl_socket, HttpResponse(HttpError{StatusCode::INTERNAL_SERVER_ERROR, parsed_request.getContentType(), "File is not open: " + response.getFilename()}));
				return;
			}

			sendResponseChunk(ssl_socket, response.getHeaders());
			if (response.isBinary())
			{
				qDebug() << "Binary stream thread";
				qint64 chunk_size = 1024;
				qint64 pos = 0;

				qDebug() << "Content type" + response.getHeaders();
				qint64 file_size = streamed_file.size();
				while(!streamed_file.atEnd())
				{
					if (pos > file_size) break;
					streamed_file.seek(pos);
					QByteArray data = streamed_file.read(chunk_size);
					pos = pos + chunk_size;					
					sendResponseChunk(ssl_socket, intToHex(data.size()).toLocal8Bit()+"\r\n");
					sendResponseChunk(ssl_socket, data.append("\r\n"));
				}
			}
			else
			{
				qDebug() << "Text stream thread";
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
		else if ((!response.getPayload().isNull()) || (parsed_request.getMethod() == RequestMethod::HEAD))
		{
			sendEntireResponse(ssl_socket, response);
			return;
		}

		sendEntireResponse(ssl_socket, HttpResponse(HttpError{StatusCode::NOT_FOUND, parsed_request.getContentType(), "This page does not exist. Check the URL and try again"}));
	}
}

void RequestWorker::handleConnection()
{
	qDebug() << "Secure connection has been established";
}

QString RequestWorker::intToHex(const int& input)
{
	return QString("%1").arg(input, 10, 16, QLatin1Char('0')).toUpper();
}

void RequestWorker::closeAndDeleteSocket(QSslSocket* socket)
{
	qDebug() << "Closing the socket";
	socket->flush();
	socket->close();
	socket->deleteLater();
}

void RequestWorker::sendResponseChunk(QSslSocket* socket, QByteArray data)
{
	// clinet cancels the stream or simply disconnects
	if (socket->state() == QSslSocket::SocketState::UnconnectedState)
	{
		qDebug() << "Socket is disconnected and no longer used";

//		emit finished();
		socket->close();
		socket->deleteLater();
//		this->quit();
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
	qDebug() << "Writing an entire response";
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
		socket->write("0\r\n");
		socket->write("\r\n");
		closeAndDeleteSocket(socket);
		return;
	}

	qDebug() << "Closing the socket forcefully";
	closeAndDeleteSocket(socket);
}
