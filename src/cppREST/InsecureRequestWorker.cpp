#include "InsecureRequestWorker.h"

InsecureRequestWorker::InsecureRequestWorker(qintptr socket)
	:
	socket_(socket)
	, is_terminated_(false)
{
}

void InsecureRequestWorker::run()
{
	qDebug() << "Start processing an incomming connection in a new separate thread";
	QTcpSocket *tcp_socket = new QTcpSocket();
	tcp_socket->setSocketOption(QAbstractSocket::KeepAliveOption, 1);

	if (!tcp_socket)
	{
		qCritical() << "Could not create a socket";
		return;
	}


	if (!tcp_socket->setSocketDescriptor(socket_))
	{
		qCritical() << "Could not set a socket descriptor";
		delete tcp_socket;
		return;
	}

	QByteArray all_request_parts;

	if (!tcp_socket->isOpen()) return;
	qDebug() << "Wait for the socket to be ready";

	bool finished_reading_headers = false;
	bool finished_reading_body = false;
	qint64 request_headers_size = 0;
	qint64 request_body_size = 0;



	while (tcp_socket->waitForReadyRead())
	{
		qDebug() << "Start the processing";

		while(tcp_socket->bytesAvailable())
		{
			QByteArray line = tcp_socket->readLine();
			if ((!finished_reading_headers) && (line.toLower().startsWith("content-length")))
			{
				QList<QByteArray> header_parts = line.trimmed().split(':');
				if (header_parts.size() > 1) request_body_size = header_parts[1].toLongLong();
			}

			all_request_parts.append(line);
			if (line.trimmed().size() == 0)
			{
				finished_reading_headers = true;
				request_headers_size = all_request_parts.size();
			}

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
		sendEntireResponse(tcp_socket, HttpResponse(ResponseStatus::INTERNAL_SERVER_ERROR, ContentType::TEXT_HTML, "Request could not be processed"));
		qDebug() << "Was not able to read from the socket. Exiting";
		return;
	}


	HttpRequest parsed_request;
	RequestPaser *parser = new RequestPaser(&all_request_parts, tcp_socket->peerAddress().toString());
	try
	{
		parsed_request = parser->getRequest();
	}
	catch (Exception& e)
	{
		sendEntireResponse(tcp_socket, HttpResponse(ResponseStatus::BAD_REQUEST, ContentType::TEXT_HTML, e.message()));
		return;
	}

	// Process the request based on the endpoint info
	qInfo() << parsed_request.methodAsString().toUpper() + "/" + parsed_request.getPath() + parsed_request.getRemoteAddress().toLatin1().data();

	Endpoint current_endpoint = EndpointManager::getEndpointByUrlAndMethod(parsed_request.getPath(), parsed_request.getMethod());
	if (current_endpoint.action_func == nullptr)
	{
		sendEntireResponse(tcp_socket, HttpResponse(ResponseStatus::BAD_REQUEST, parsed_request.getContentType(), "This action cannot be processed"));
		return;
	}

	try
	{
		EndpointManager::validateInputData(&current_endpoint, parsed_request);
	}
	catch (ArgumentException& e)
	{
		qDebug() << "Parameter validation has failed";
		sendEntireResponse(tcp_socket, HttpResponse(ResponseStatus::BAD_REQUEST, parsed_request.getContentType(), e.message()));
		return;
	}

	qDebug() << "Requested:" + current_endpoint.comment;
	if (current_endpoint.is_password_protected)
	{
		qDebug() << "Accessing password protected area";
		HttpResponse auth_response = EndpointManager::blockInvalidUsers(parsed_request);
		if (auth_response.getStatusCode() > 0)
		{
			sendEntireResponse(tcp_socket, auth_response);
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
		qDebug() << "Error while executing an action" << e.message();
		sendEntireResponse(tcp_socket, HttpResponse(ResponseStatus::INTERNAL_SERVER_ERROR, parsed_request.getContentType(), "Could not process endpoint action"));
		return;
	}

	if (response.isStream())
	{
		qDebug() << "Initiating a stream: " + response.getFilename();

		if (response.getFilename().isEmpty())
		{
			HttpResponse error_response;
			sendEntireResponse(tcp_socket, HttpResponse(ResponseStatus::NOT_FOUND, parsed_request.getContentType(), "File name has not been found"));
			return;
		}

		QFile streamed_file(response.getFilename());
		if (!streamed_file.exists())
		{
			sendEntireResponse(tcp_socket, HttpResponse(ResponseStatus::NOT_FOUND, parsed_request.getContentType(), "Requested file does not exist"));
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
			sendEntireResponse(tcp_socket, HttpResponse(ResponseStatus::INTERNAL_SERVER_ERROR, parsed_request.getContentType(), "Could not open a file for streaming: " + response.getFilename()));
			return;
		}

		if (!streamed_file.isOpen())
		{
			qDebug() << "File is not open";
			sendEntireResponse(tcp_socket, HttpResponse(ResponseStatus::INTERNAL_SERVER_ERROR, parsed_request.getContentType(), "File is not open: " + response.getFilename()));
			return;
		}

		sendResponseDataPart(tcp_socket, response.getStatusLine());
		sendResponseDataPart(tcp_socket, response.getHeaders());

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
				sendResponseDataPart(tcp_socket, intToHex(data.size()).toLocal8Bit()+"\r\n");
				sendResponseDataPart(tcp_socket, data.append("\r\n"));
			}
			else
			{
				// Keep connection alive and add data incrementally in parts (content-lenght is specified)
				sendResponseDataPart(tcp_socket, data);
			}
		}

		streamed_file.close();

		// Should be used for chunked transfer (without content-lenght)
		if (parsed_request.getHeaderByName("Transfer-Encoding").toLower() == "chunked")
		{
			sendResponseDataPart(tcp_socket, "0\r\n");
			sendResponseDataPart(tcp_socket, "\r\n");
		}

		finishPartialDataResponse(tcp_socket);
		return;
	}
	else if (!response.getPayload().isNull())
	{
		sendEntireResponse(tcp_socket, response);
		return;
	}
	// Returns headers with file size without fetching the file itself
	else if (parsed_request.getMethod() == RequestMethod::HEAD)
	{
		sendEntireResponse(tcp_socket, response);
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
		sendEntireResponse(tcp_socket, response);
		return;
	}
	else if (response.getPayload().isNull())
	{
		sendEntireResponse(tcp_socket, HttpResponse(ResponseStatus::INTERNAL_SERVER_ERROR, parsed_request.getContentType(), "Could not produce any output"));
		return;
	}

	sendEntireResponse(tcp_socket, HttpResponse(ResponseStatus::NOT_FOUND, parsed_request.getContentType(), "This page does not exist. Check the URL and try again"));
}

void InsecureRequestWorker::handleConnection()
{
	qDebug() << "HTTP connection has been established";
}

void InsecureRequestWorker::socketDisconnected()
{
	qDebug() << "Client has disconnected from the socket";
}

QString InsecureRequestWorker::intToHex(const int& input)
{
	return QString("%1").arg(input, 10, 16, QLatin1Char('0')).toUpper();
}

void InsecureRequestWorker::closeAndDeleteSocket(QTcpSocket* socket)
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

void InsecureRequestWorker::sendResponseDataPart(QTcpSocket* socket, QByteArray data)
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

void InsecureRequestWorker::sendEntireResponse(QTcpSocket* socket, HttpResponse response)
{
	qDebug() << "Writing an entire response";
	if (socket->state() != QSslSocket::SocketState::UnconnectedState)
	{
		socket->write(response.getStatusLine());
		socket->write(response.getHeaders());
		socket->write(response.getPayload());
	}
	closeAndDeleteSocket(socket);
}

void InsecureRequestWorker::finishPartialDataResponse(QTcpSocket* socket)
{
	if ((socket->state() != QSslSocket::SocketState::UnconnectedState) && (socket->bytesToWrite()))
	{
		socket->flush();
		socket->waitForBytesWritten();
	}

	closeAndDeleteSocket(socket);
}
