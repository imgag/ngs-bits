#include "RequestWorker.h"

RequestWorker::RequestWorker(QSslConfiguration ssl_configuration, qintptr socket)
	:
	 ssl_configuration_(ssl_configuration)
	, socket_(socket)
   , is_terminated_(false)
{
}

void RequestWorker::run()
{
	qDebug() << "Start processing an incomming connection in a new separate thread";
	QSslSocket *ssl_socket = new QSslSocket();
	ssl_socket->setSocketOption(QAbstractSocket::KeepAliveOption, 1);

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
	connect(ssl_socket, &QSslSocket::disconnected, this, &RequestWorker::socketDisconnected);
	connect(this, SIGNAL(securelyConnected()), this, SLOT(handleConnection()));

	qDebug() << "Starting the encryption";
	ssl_socket->startServerEncryption();

	QByteArray all_request_parts;

	if (!ssl_socket->isOpen()) return;
	qDebug() << "Wait for the socket to be ready";	

	bool finished_reading_headers = false;
	bool finished_reading_body = false;
	qint64 request_headers_size = 0;
	qint64 request_body_size = 0;

	while (ssl_socket->waitForReadyRead())
	{
		qDebug() << "Start the processing";
		if (!ssl_socket->isEncrypted())
		{
			closeAndDeleteSocket(ssl_socket);
			return;
		}

		while(ssl_socket->bytesAvailable())
		{
			QByteArray line = ssl_socket->readLine();
			if ((!finished_reading_headers) && (line.toLower().startsWith("content-length")))
			{
				QList<QByteArray> header_parts = line.trimmed().split(':');
				if (header_parts.size() > 1) request_body_size = header_parts.value(1).toInt();
			}

			all_request_parts.append(line);
			if (line.trimmed().size() == 0)
			{
				finished_reading_headers = true;
				request_headers_size = all_request_parts.size();
			}

//			qDebug() << line << " = " << finished_reading_headers;
//			qDebug() << "all_request_parts.size() " << all_request_parts.size();
//			qDebug() << "requestHeadersSize " << request_headers_size;
//			qDebug() << "requestBodySize " << request_body_size;
			if ((finished_reading_headers) && ((all_request_parts.size() - request_headers_size) >= request_body_size))
			{
				finished_reading_body = true;
				break;
			}

		}

		if (finished_reading_body) break;
	}

	if (all_request_parts.size() == 0)
	{
		sendEntireResponse(ssl_socket, HttpResponse(ResponseStatus::INTERNAL_SERVER_ERROR, ContentType::TEXT_HTML, "Request could not be processed"));
		qDebug() << "Was not able to read from the socket. Exiting";
		return;
	}


	HttpRequest parsed_request;
	RequestPaser *parser = new RequestPaser(&all_request_parts, ssl_socket->peerAddress().toString());
	try
	{
		parsed_request = parser->getRequest();
	}
	catch (Exception& e)
	{
		sendEntireResponse(ssl_socket, HttpResponse(ResponseStatus::BAD_REQUEST, ContentType::TEXT_HTML, e.message()));
		return;
	}

	// Process the request based on the endpoint info
	qInfo() << parsed_request.methodAsString().toUpper() + "/" + parsed_request.getPath() + parsed_request.getRemoteAddress().toLatin1().data();

	Endpoint current_endpoint = EndpointManager::getEndpointByUrlAndMethod(parsed_request.getPath(), parsed_request.getMethod());
	if (current_endpoint.action_func == nullptr)
	{
		sendEntireResponse(ssl_socket, HttpResponse(ResponseStatus::BAD_REQUEST, parsed_request.getContentType(), "This action cannot be processed"));
		return;
	}

	try
	{
		EndpointManager::validateInputData(&current_endpoint, parsed_request);
	}
	catch (ArgumentException& e)
	{
		qDebug() << "Parameter validation has failed";
		sendEntireResponse(ssl_socket, HttpResponse(ResponseStatus::BAD_REQUEST, parsed_request.getContentType(), e.message()));
		return;
	}

	qDebug() << "Requested:" + current_endpoint.comment;
	if (current_endpoint.is_password_protected)
	{
		qDebug() << "Accessing password protected area";
		HttpResponse auth_response = EndpointManager::blockInvalidUsers(parsed_request);
		qDebug() << "Response status line = " << auth_response.getStatusLine();
		if (auth_response.getStatusCode() > 0)
		{
			sendEntireResponse(ssl_socket, auth_response);
			return;
		}
	}

	HttpResponse (*endpoint_action_)(const HttpRequest& request) = current_endpoint.action_func;
	HttpResponse response;

	try
	{
		response = (*endpoint_action_)(parsed_request);
	}
	catch (Exception& e)
	{
		qDebug() << "Error while executing an action";
		sendEntireResponse(ssl_socket, HttpResponse(ResponseStatus::INTERNAL_SERVER_ERROR, parsed_request.getContentType(), "Could not process endpoint action"));
		return;
	}

	if (response.isStream())
	{
		qDebug() << "Initiating a stream: " + response.getFilename();

		if (response.getFilename().isEmpty())
		{
			HttpResponse error_response;
			sendEntireResponse(ssl_socket, HttpResponse(ResponseStatus::NOT_FOUND, parsed_request.getContentType(), "File name has not been found"));
			return;
		}

		QFile streamed_file(response.getFilename());
		if (!streamed_file.exists())
		{
			sendEntireResponse(ssl_socket, HttpResponse(ResponseStatus::NOT_FOUND, parsed_request.getContentType(), "Requested file does not exist"));
			return;
		}

		QFile::OpenMode mode = QFile::ReadOnly;

		try
		{
			streamed_file.open(mode);
		}
		catch (Exception& e)
		{
			qDebug() << "Error while opening a file for streaming";
			sendEntireResponse(ssl_socket, HttpResponse(ResponseStatus::INTERNAL_SERVER_ERROR, parsed_request.getContentType(), "Could not open a file for streaming: " + response.getFilename()));
			return;
		}

		if (!streamed_file.isOpen())
		{
			qDebug() << "File is not open";
			sendEntireResponse(ssl_socket, HttpResponse(ResponseStatus::INTERNAL_SERVER_ERROR, parsed_request.getContentType(), "File is not open: " + response.getFilename()));
			return;
		}

		sendResponseDataPart(ssl_socket, response.getStatusLine());
		sendResponseDataPart(ssl_socket, response.getHeaders());

		qint64 chunk_size = 1024*10;
		qint64 pos = 0;
		qint64 file_size = streamed_file.size();
		while(!streamed_file.atEnd())
		{
			if (is_terminated_) break;
			if ((pos > file_size) || (pos < 0)) break;
			streamed_file.seek(pos);
			QByteArray data = streamed_file.read(chunk_size);
			pos = pos + chunk_size;

			if (parsed_request.getHeaderByName("Transfer-Encoding").toLower() == "chunked")
			{
				// Should be used for chunked transfer (without content-lenght)
				sendResponseDataPart(ssl_socket, intToHex(data.size()).toLocal8Bit()+"\r\n");
				sendResponseDataPart(ssl_socket, data.append("\r\n"));
			}
			else
			{
				// Keep connection alive and add data incrementally in parts (content-lenght is specified)
				sendResponseDataPart(ssl_socket, data);
			}
		}

		streamed_file.close();

		// Should be used for chunked transfer (without content-lenght)
		if (parsed_request.getHeaderByName("Transfer-Encoding").toLower() == "chunked")
		{
			sendResponseDataPart(ssl_socket, "0\r\n");
			sendResponseDataPart(ssl_socket, "\r\n");
		}

		finishPartialDataResponse(ssl_socket);
		return;
	}
	else if (!response.getPayload().isNull())
	{
		sendEntireResponse(ssl_socket, response);
		return;
	}
	// Returns headers with file size without fetching the file itself
	else if (parsed_request.getMethod() == RequestMethod::HEAD)
	{
		sendEntireResponse(ssl_socket, response);
		return;
	}
	else if ((response.getPayload().isNull()) && (parsed_request.getHeaders().contains("range")))
	{
		// Fetching non-existing range (e.g. larger than the file itself)
		BasicResponseData response_data;
		response_data.filename = response.getFilename();
		response_data.file_size = QFile(response.getFilename()).size();
		response.setStatus(ResponseStatus::RANGE_NOT_SATISFIABLE);
		response.setRangeNotSatisfiableHeaders(response_data);
		sendEntireResponse(ssl_socket, response);
		return;
	}
	else if (response.getPayload().isNull())
	{
		sendEntireResponse(ssl_socket, HttpResponse(ResponseStatus::INTERNAL_SERVER_ERROR, parsed_request.getContentType(), "Could not produce any output"));
		return;
	}

	sendEntireResponse(ssl_socket, HttpResponse(ResponseStatus::NOT_FOUND, parsed_request.getContentType(), "This page does not exist. Check the URL and try again"));

	qDebug() << "Entire request" << all_request_parts;
}

void RequestWorker::handleConnection()
{
	qDebug() << "Secure connection has been established";
}

void RequestWorker::socketDisconnected()
{
	qDebug() << "Client has disconnected from the socket";
}

QString RequestWorker::intToHex(const int& input)
{
	return QString("%1").arg(input, 10, 16, QLatin1Char('0')).toUpper();
}

void RequestWorker::closeAndDeleteSocket(QSslSocket* socket)
{
	qDebug() << "Closing the socket";
	is_terminated_ = true;
	if (socket->state() != QSslSocket::SocketState::UnconnectedState)
	{
		socket->flush();
		socket->waitForBytesWritten();
	}
	socket->close();
	socket->deleteLater();
}

void RequestWorker::sendResponseDataPart(QSslSocket* socket, QByteArray data)
{
	// clinet completes/cancels the stream or simply disconnects
	if (socket->state() == QSslSocket::SocketState::UnconnectedState)
	{
		closeAndDeleteSocket(socket);
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
	socket->write(response.getStatusLine());
	socket->write(response.getHeaders());
	socket->write(response.getPayload());
	closeAndDeleteSocket(socket);
}

void RequestWorker::finishPartialDataResponse(QSslSocket* socket)
{
	if ((socket->state() != QSslSocket::SocketState::UnconnectedState) && (socket->bytesToWrite()))
	{
		socket->flush();		
		socket->waitForBytesWritten();
	}

	closeAndDeleteSocket(socket);
}
