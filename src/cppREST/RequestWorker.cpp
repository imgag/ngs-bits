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
	QSslSocket *ssl_socket = new QSslSocket();
	ssl_socket->setSocketOption(QAbstractSocket::KeepAliveOption, 1);

	if (!ssl_socket)
	{
		Log::error("Could not create a socket: " + ssl_socket->errorString());
		return;
	}
	ssl_socket->setSslConfiguration(ssl_configuration_);

	if (!ssl_socket->setSocketDescriptor(socket_))
	{
		Log::error("Could not set a socket descriptor: " + ssl_socket->errorString());
		ssl_socket->deleteLater();
		return;
	}

	typedef void (QSslSocket::* sslFailed)(const QList<QSslError> &);
	connect(ssl_socket, static_cast<sslFailed>(&QSslSocket::sslErrors), this, &RequestWorker::sslFailed);
	connect(ssl_socket, &QSslSocket::peerVerifyError, this, &RequestWorker::verificationFailed);
	connect(ssl_socket, &QSslSocket::encrypted, this, &RequestWorker::securelyConnected);
	connect(ssl_socket, &QSslSocket::disconnected, this, &RequestWorker::socketDisconnected);
	connect(this, SIGNAL(securelyConnected()), this, SLOT(handleConnection()));

	try
	{


		ssl_socket->startServerEncryption();

		QByteArray all_request_parts;

		if (!ssl_socket->isOpen())
		{
			Log::error("Could not open the socket for data exchange: " + ssl_socket->errorString());
			ssl_socket->deleteLater();
			return;
		}

		bool finished_reading_headers = false;
		bool finished_reading_body = false;
		qint64 request_headers_size = 0;
		qint64 request_body_size = 0;

		while (ssl_socket->waitForReadyRead())
		{
			ssl_socket->waitForEncrypted();

			if (!ssl_socket->isEncrypted() || (ssl_socket->state() == QSslSocket::SocketState::UnconnectedState))
			{
				Log::error("Connection cannot be continued: " + ssl_socket->errorString());
				closeAndDeleteSocket(ssl_socket);
				return;
			}

			while(ssl_socket->bytesAvailable())
			{
				QByteArray line = ssl_socket->readLine();
				if ((!finished_reading_headers) && (line.toLower().startsWith("content-length")))
				{
					QList<QByteArray> header_parts = line.trimmed().split(':');
					if (header_parts.size() > 1) request_body_size = header_parts[1].toLongLong();
				}

				all_request_parts.append(line);
				if ((line.trimmed().size() == 0) && (!finished_reading_headers))
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
			sendEntireResponse(ssl_socket, HttpResponse(ResponseStatus::INTERNAL_SERVER_ERROR, ContentType::TEXT_PLAIN, "Request could not be processed"));
			Log::error("Was not able to read from the socket. Exiting. " + ssl_socket->errorString());
			return;
		}

		HttpRequest parsed_request;
		RequestParser *parser = new RequestParser();
		try
		{
			parsed_request = parser->parse(&all_request_parts);
		}
		catch (Exception& e)
		{
			Log::error("Could not parse the request: " + e.message());
			sendEntireResponse(ssl_socket, HttpResponse(ResponseStatus::BAD_REQUEST, ContentType::TEXT_HTML, e.message()));
			return;
		}

		ContentType error_type = HttpUtils::detectErrorContentType(parsed_request.getHeaderByName("User-Agent"));

		// Process the request based on the endpoint info
		Endpoint current_endpoint = EndpointManager::getEndpointByUrlAndMethod(parsed_request.getPath(), parsed_request.getMethod());
		if (current_endpoint.action_func == nullptr)
		{
			sendEntireResponse(ssl_socket, HttpResponse(ResponseStatus::BAD_REQUEST, error_type, "This action cannot be processed"));
			return;
		}

		try
		{
			EndpointManager::validateInputData(&current_endpoint, parsed_request);
		}
		catch (ArgumentException& e)
		{
			Log::warn("Parameter validation has failed: " + e.message());
			sendEntireResponse(ssl_socket, HttpResponse(ResponseStatus::BAD_REQUEST, error_type, e.message()));
			return;
		}

		QString user_token = EndpointManager::getTokenIfAvailable(parsed_request);
		QString user_info;
		if (!user_token.isEmpty())
		{
			Session user_session = SessionManager::getSessionBySecureToken(user_token);
			if (!user_session.isEmpty())
			{
				try
				{
					QString user_login = NGSD().userLogin(user_session.user_id);
					QString user_name = NGSD().userName(user_session.user_id);
					user_info = " - requested by " + user_login + " (" + user_name + ")";
				}
				catch (DatabaseException& e)
				{
					Log::error("Database request failed: " + e.message());
					user_info = " - requested by unknown user";
				}
			}
		}
		QString client_type = "Unknown client";
		if (!parsed_request.getHeaderByName("User-Agent").isEmpty())
		{
			if (parsed_request.getHeaderByName("User-Agent")[0].toLower().indexOf("igv") > -1)
			{
				client_type = "IGV";
			}
			else if ((parsed_request.getHeaderByName("User-Agent")[0].toLower().indexOf("gsvar") > -1) || (parsed_request.getHeaderByName("User-Agent")[0].toLower().indexOf("qt") > -1))
			{
				client_type = "GSvar";
			}
			else
			{
				client_type = "Browser";
			}
		}
		client_type = " - " + client_type;

		if (current_endpoint.authentication_type != AuthType::NONE)
		{
			HttpResponse auth_response;

			if (current_endpoint.authentication_type == AuthType::HTTP_BASIC_AUTH) auth_response = EndpointManager::getBasicHttpAuthStatus(parsed_request);
			if (current_endpoint.authentication_type == AuthType::USER_TOKEN) auth_response = EndpointManager::getUserTokenAuthStatus(parsed_request);
			if (current_endpoint.authentication_type == AuthType::DB_TOKEN) auth_response = EndpointManager::getDbTokenAuthStatus(parsed_request);

			if (auth_response.getStatus() != ResponseStatus::OK)
			{
				Log::error("Token check failed: response code " + QString::number(HttpUtils::convertResponseStatusToStatusCodeNumber(auth_response.getStatus())) + user_info + client_type);
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
			Log::error("Error while executing an action: " + e.message());
			sendEntireResponse(ssl_socket, HttpResponse(ResponseStatus::INTERNAL_SERVER_ERROR, error_type, "Could not process endpoint action: " + e.message()));
			return;
		}


		Log::info(HttpUtils::convertMethodTypeToString(current_endpoint.method).toUpper() + " " + current_endpoint.url + " - " + current_endpoint.comment + user_info + client_type);

		if (response.isStream())
		{
			Log::info("Initiating a stream: " + response.getFilename() + user_info + client_type);

			if (response.getFilename().isEmpty())
			{
				HttpResponse error_response;
				QString error_message = "Streaming request contains an empty file name";
				Log::error(error_message + user_info + client_type);
				sendEntireResponse(ssl_socket, HttpResponse(ResponseStatus::NOT_FOUND, error_type, error_message));
				return;
			}

			QSharedPointer<QFile> streamed_file = QSharedPointer<QFile>(new QFile(response.getFilename()));
			if (!streamed_file.data()->exists())
			{
				QString error_message = "Requested file does not exist: " + response.getFilename();
				Log::error(error_message + user_info + client_type);
				sendEntireResponse(ssl_socket, HttpResponse(ResponseStatus::NOT_FOUND, error_type, error_message));
				return;
			}

			if (!streamed_file.data()->open(QFile::ReadOnly))
			{
				QString error_message = "Could not open a file for streaming: " + response.getFilename();
				Log::error(error_message + user_info + client_type);
				sendEntireResponse(ssl_socket, HttpResponse(ResponseStatus::INTERNAL_SERVER_ERROR, error_type, error_message));
				return;
			}

			if (!streamed_file.data()->isOpen())
			{
				QString error_message = "File is not open: " + response.getFilename();
				Log::error(error_message + user_info + client_type);
				sendEntireResponse(ssl_socket, HttpResponse(ResponseStatus::INTERNAL_SERVER_ERROR, error_type, error_message));
				return;
			}

			sendResponseDataPart(ssl_socket, response.getStatusLine());
			sendResponseDataPart(ssl_socket, response.getHeaders());

			quint64 pos = 0;
			quint64 file_size = streamed_file.data()->size();
			bool transfer_encoding_chunked = false;

			if (!parsed_request.getHeaderByName("Transfer-Encoding").isEmpty())
			{
				if (parsed_request.getHeaderByName("Transfer-Encoding")[0].toLower() == "chunked")
				{
					transfer_encoding_chunked = true;
				}
			}

			long chunk_size = STREAM_CHUNK_SIZE;
			QByteArray data;
			QList<ByteRange> ranges = response.getByteRanges();
			int ranges_count = ranges.count();

			// Range request
			for (int i = 0; i < ranges_count; ++i)
			{
				chunk_size = STREAM_CHUNK_SIZE;
				pos = ranges[i].start;
				if (ranges_count > 1)
				{
					sendResponseDataPart(ssl_socket, "--"+response.getBoundary()+"\r\n");
					sendResponseDataPart(ssl_socket, "Content-Type: application/octet-stream\r\n");
					sendResponseDataPart(ssl_socket, "Content-Range: bytes " + QByteArray::number(ranges[i].start) + "-" + QByteArray::number(ranges[i].end) + "/" + QByteArray::number(file_size) + "\r\n");
					sendResponseDataPart(ssl_socket, "\r\n");
				}
				while(pos<(ranges[i].end+1))
				{
					if ((is_terminated_) || (ssl_socket->state() == QSslSocket::SocketState::UnconnectedState) || (ssl_socket->state() == QSslSocket::SocketState::ClosingState))
					{
						Log::info("Range streaming request process has been terminated: " + response.getFilename() + user_info + client_type);
						streamed_file.data()->close();
						return;
					}

					if (pos >= (file_size-1)) break;
					streamed_file.data()->seek(pos);

					if ((pos+chunk_size)>(ranges[i].end+1))
					{
						chunk_size = ranges[i].end - pos + 1;
					}

					if (chunk_size <= 0) break;
					data = streamed_file.data()->read(chunk_size);
					sendResponseDataPart(ssl_socket, data);
					pos = pos + data.size();
				}
				if (is_terminated_) return;
				if (ranges_count > 1) sendResponseDataPart(ssl_socket, "\r\n");
				if ((i == (ranges_count-1)) && (ranges_count > 1))
				{
					sendResponseDataPart(ssl_socket, "--" + response.getBoundary() + "--\r\n");
				}

			}

			// Regular stream
			if (ranges_count == 0)
			{
				while(!streamed_file.data()->atEnd())
				{
					if ((pos > file_size) || (is_terminated_) || (ssl_socket->state() == QSslSocket::SocketState::UnconnectedState) || (ssl_socket->state() == QSslSocket::SocketState::ClosingState))
					{
						Log::info("Streaming request process has been terminated: " + response.getFilename() + user_info + client_type);
						streamed_file.data()->close();
						return;
					}

					streamed_file.data()->seek(pos);
					data = streamed_file.data()->read(chunk_size);
					pos = pos + chunk_size;

					if (transfer_encoding_chunked)
					{
						// Should be used for chunked transfer (without content-lenght)
						sendResponseDataPart(ssl_socket, intToHex(data.size()).toUtf8()+"\r\n");
						sendResponseDataPart(ssl_socket, data.append("\r\n"));
					}
					else
					{
						// Keep connection alive and add data incrementally in parts (content-lenght is specified)
						sendResponseDataPart(ssl_socket, data);
					}
				}
			}

			streamed_file.data()->close();

			// Should be used for chunked transfer (without content-lenght)
			if (transfer_encoding_chunked)
			{
				sendResponseDataPart(ssl_socket, "0\r\n");
				sendResponseDataPart(ssl_socket, "\r\n");
			}

			closeAndDeleteSocket(ssl_socket);
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
			QString error_message = "Could not produce any output";
			Log::error(error_message + user_info + client_type);
			sendEntireResponse(ssl_socket, HttpResponse(ResponseStatus::INTERNAL_SERVER_ERROR, error_type, error_message));
			return;
		}

		QString error_message = "The requested resource does not exist: " + parsed_request.getPath() + ". Check the URL and try again";
		Log::error(error_message + user_info + client_type);
		sendEntireResponse(ssl_socket, HttpResponse(ResponseStatus::NOT_FOUND, error_type, error_message));
	}
	catch (...)
	{
		QString error_message = "Unexpected error inside the request worker. See logs for more details";
		Log::error(error_message);
		sendEntireResponse(ssl_socket, HttpResponse(ResponseStatus::INTERNAL_SERVER_ERROR, ContentType::TEXT_PLAIN, error_message));
	}
}

void RequestWorker::handleConnection()
{
	Log::info("Secure connection has been established");
}

void RequestWorker::socketDisconnected()
{
	Log::info("Client has disconnected from the socket");
	exit(0);
}

QString RequestWorker::intToHex(const int& input)
{
	return QString("%1").arg(input, 10, 16, QLatin1Char('0')).toUpper();
}

void RequestWorker::closeAndDeleteSocket(QSslSocket* socket)
{
	is_terminated_ = true;

	if ((socket->state() == QSslSocket::SocketState::UnconnectedState) || (socket->state() == QSslSocket::SocketState::ClosingState))
	{
		exit(0);
	}
	else
	{
		if (socket->bytesToWrite()) socket->waitForBytesWritten(5000);
		socket->disconnect();
		socket->disconnectFromHost();
		socket->close();
		socket->deleteLater();
	}
}

void RequestWorker::sendResponseDataPart(QSslSocket* socket, QByteArray data)
{
	if (socket->state() != QSslSocket::SocketState::UnconnectedState)
	{
		socket->write(data, data.size());
		if (socket->bytesToWrite()) socket->waitForBytesWritten(5000);
	}
}

void RequestWorker::sendEntireResponse(QSslSocket* socket, HttpResponse response)
{
	if (response.getStatusCode() > 200) Log::warn("The server returned " + QString::number(response.getStatusCode()) + " - " + HttpUtils::convertResponseStatusToReasonPhrase(response.getStatus()));
	if (socket->state() != QSslSocket::SocketState::UnconnectedState)
	{
		socket->write(response.getStatusLine());
		socket->write(response.getHeaders());
		socket->write(response.getPayload());
	}
	closeAndDeleteSocket(socket);
}
