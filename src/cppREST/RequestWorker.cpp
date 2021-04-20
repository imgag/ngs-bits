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
		THROW(Exception, "Could not create a socket");
		return;
	}
	ssl_socket->setSslConfiguration(ssl_configuration_);

	if (!ssl_socket->setSocketDescriptor(socket_))
	{
		THROW(Exception, "Could not set a socket descriptor");
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
		ssl_socket->write(HttpResponse(HttpError{StatusCode::INTERNAL_SERVER_ERROR, ContentType::TEXT_HTML, "Request could not be processed"}));
		ssl_socket->close();
		ssl_socket->deleteLater();
	}
	else
	{

		QByteArray raw_request = ssl_socket->readAll();
		qDebug() << raw_request;
		HttpRequest parsed_request;
		RequestPaser *parser = new RequestPaser(&raw_request, ssl_socket->peerAddress().toString());
		try
		{
			parsed_request = parser->getRequest();
		}
		catch (ArgumentException& e)
		{
			ssl_socket->write(HttpResponse(HttpError{StatusCode::BAD_REQUEST, ContentType::TEXT_HTML, e.message()}));
			return;
		}

//		ssl_socket->write(HttpResponse(HttpError{StatusCode::OK, ContentType::TEXT_HTML, "Empty page"}));










		qInfo().noquote() << parsed_request.methodAsString().toUpper() << "/" << parsed_request.getPath() << parsed_request.getRemoteAddress().toLatin1().data();

		Endpoint current_endpoint = EndpointManager::getEndpointEntity(parsed_request.getPath(), parsed_request.getMethod());
		if (current_endpoint.action_func == nullptr)
		{
			allDataReady(ssl_socket, HttpResponse(HttpError{StatusCode::BAD_REQUEST, parsed_request.getContentType(), "This action cannot be processed"}));
			return;
		}

		try
		{
			EndpointManager::validateInputData(&current_endpoint, parsed_request);
		}
		catch (ArgumentException& e)
		{
			allDataReady(ssl_socket, HttpResponse(HttpError{StatusCode::BAD_REQUEST, parsed_request.getContentType(), e.message()}));
			return;
		}

		ServerHelper::debug("Requested:" + current_endpoint.comment);

		HttpResponse (*endpoint_action_)(HttpRequest request) = current_endpoint.action_func;
		HttpResponse response = (*endpoint_action_)(parsed_request);

		if (response.isStream())
		{
			dataChunkReady(ssl_socket, response.getHeaders());
			QFile streamed_file(response.getFilename());
			if (response.isBinary())
			{
				streamed_file.open(QFile::ReadOnly);
			}
			else
			{
				streamed_file.open(QFile::ReadOnly | QFile::Text);
			}

			if (!streamed_file.isOpen())
			{
				allDataReady(ssl_socket, HttpResponse(HttpError{StatusCode::INTERNAL_SERVER_ERROR, parsed_request.getContentType(), "Could not open file for reading"}));
				return;
			}

			if (response.isBinary())
			{
				ServerHelper::debug("Binary stream thread");
				qint64 chunk_size = 1024;
				qint64 pos = 0;

				while(!streamed_file.atEnd())
				{
					streamed_file.seek(pos);
					QByteArray data = streamed_file.read(chunk_size);
					pos = pos + chunk_size;
					dataChunkReady(ssl_socket, intToHex(data.length()).toLocal8Bit()+"\r\n");
					dataChunkReady(ssl_socket, data.append("\r\n"));
				}
			}
			else
			{
				ServerHelper::debug("Text stream thread");
				QTextStream stream(&streamed_file);
				while(!stream.atEnd())
				{
					QByteArray line = stream.readLine().append("\n").toLocal8Bit();
					dataChunkReady(ssl_socket, intToHex(line.size()).toLocal8Bit()+"\r\n");
					dataChunkReady(ssl_socket, line+"\r\n");
				}
			}
			streamed_file.close();

			dataChunkReady(ssl_socket, "0\r\n");
			dataChunkReady(ssl_socket, "\r\n");
			dataChunkReady(ssl_socket, "%end%\r\n");
			return;
		}
		else if (!response.getPayload().isNull())
		{
			allDataReady(ssl_socket, response);
			return;
		}

		allDataReady(ssl_socket, HttpResponse(HttpError{StatusCode::NOT_FOUND, parsed_request.getContentType(), "This page does not exist. Check the URL and try again"}));
















//		ssl_socket->close();
//		ssl_socket->deleteLater();
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

void RequestWorker::dataChunkReady(QSslSocket* socket, QByteArray data)
{
	if (socket->bytesToWrite())
	{
		socket->waitForBytesWritten();
	}

	QString string_data = QString(data).trimmed();
	if (string_data != "%end%") socket->write(data);

	if (string_data == "%end%")
	{
		if (!socket->bytesToWrite())
		{
			ServerHelper::debug("Closing the socket");
			socket->close();
			socket->deleteLater();
		}
		else {
			ServerHelper::debug("Cannot close the socket, the server is still sending the data");
			socket->waitForBytesWritten();
			ServerHelper::debug("Closing the socket forcefully");
			socket->close();
			socket->deleteLater();
		}
	}
}

void RequestWorker::allDataReady(QSslSocket* socket, HttpResponse response)
{
	socket->write(response.getHeaders());
	socket->write(response.getPayload());
	socket->flush();
	socket->close();
	socket->deleteLater();
}
