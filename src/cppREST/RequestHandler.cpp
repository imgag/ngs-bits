#include "RequestHandler.h"

static qint64 MAX_REQUEST_LENGTH = 2048; // for the IE compatibility

RequestHandler::RequestHandler(QSslSocket *sock)
	: socket_(sock)
	, end_of_content_found_(false)
{
	qRegisterMetaType<HttpResponse>();
	connect(sock, SIGNAL(readyRead()), this, SLOT(dataReceived()));
}

RequestMethod RequestHandler::inferRequestMethod(QByteArray in)
{
	if (in.toUpper() == QByteArrayLiteral("GET"))
	{
		return RequestMethod::GET;
	}
	if (in.toUpper() == QByteArrayLiteral("POST"))
	{
		return RequestMethod::POST;
	}
	if (in.toUpper() == QByteArrayLiteral("DELETE"))
	{
		return RequestMethod::DELETE;
	}
	if (in.toUpper() == QByteArrayLiteral("PUT"))
	{
		return RequestMethod::PUT;
	}
	if (in.toUpper() == QByteArrayLiteral("PATCH"))
	{
		return RequestMethod::PATCH;
	}

	THROW(ArgumentException, "Incorrect request method");
}

QList<QByteArray> RequestHandler::getRequestBody()
{
	QList<QByteArray> request_items;

	if (socket_->canReadLine())
	{
		request_items = socket_->readAll().split('\n');
		for (int i = 0; i < request_items.count(); ++i)
		{
			request_items[i] = request_items[i].trimmed();
		}
	}
	else if (socket_->bytesAvailable() > MAX_REQUEST_LENGTH)
	{
		writeResponse(HttpResponse(HttpError{StatusCode::BAD_REQUEST, ContentType::TEXT_HTML, "Maximum request lenght has been exceeded"}));
	}

	return request_items;
}

QList<QByteArray> RequestHandler::getKeyValuePair(QByteArray in)
{
	QList<QByteArray> result {};

	if (in.indexOf('=')>-1)
	{
		result = in.split('=');
	}

	return result;
}

QMap<QString, QString> RequestHandler::getVariables(QByteArray in)
{
	QMap<QString, QString> url_vars {};
	QList<QByteArray> var_list = in.split('&');
	QByteArray cur_key {};

	for (int i = 0; i < var_list.count(); ++i)
	{
		QList<QByteArray> pair = getKeyValuePair(var_list[i]);
		if (pair.length()==2)
		{
			url_vars.insert(pair[0], pair[1]);
		}
	}

	return url_vars;
}

QByteArray RequestHandler::getVariableSequence(QByteArray url)
{
	QByteArray var_string {};
	if (url.indexOf('?') == -1) return var_string;
	return url.split('?')[1];
}

QString RequestHandler::getRequestPrefix(QList<QString> path_items)
{
	if (path_items.count()>1)
	{
		return ServerHelper::getUrlWithoutParams(path_items[1]);
	}
	return "";
}

QString RequestHandler::getRequestPath(QList<QString> path_items)
{
	if (path_items.count()>2)
	{		
		return ServerHelper::getUrlWithoutParams(path_items[2]);
	}
	return "";
}

QList<QString> RequestHandler::getRequestPathParams(QList<QString> path_items)
{
	QList<QString> params {};
	if (path_items.count()>3)
	{
		for (int p = 3; p < path_items.count(); ++p)
		{
			if (!path_items[p].trimmed().isEmpty())
			{
				params.append(path_items[p].trimmed());
			}
		}
	}
	return params;
}

void RequestHandler::readRequest(QList<QByteArray> body)
{
	HttpRequest request {};
	request.setRemoteAddress(socket_->peerAddress().toString());

	for (int i = 0; i < body.count(); ++i)
	{
		// First line with method type and URL
		if (i == 0)
		{
			QList<QByteArray> request_info = body[i].split(' ');
			if (request_info.length() < 2)
			{
				writeResponse(HttpResponse(HttpError{StatusCode::BAD_REQUEST, ContentType::TEXT_HTML, "Cannot process the request. It is possible a URL is missing or incorrect"}));
				return;
			}
			try
			{
				request.setMethod(inferRequestMethod(request_info[0].toUpper()));
			}
			catch (ArgumentException& e)
			{
				writeResponse(HttpResponse(HttpError{StatusCode::BAD_REQUEST, ContentType::TEXT_HTML, e.message()}));
				return;
			}


			QList<QString> path_items = QString(request_info[1]).split('/');

			request.setPrefix(getRequestPrefix(path_items));
			request.setPath(getRequestPath(path_items));
			request.setPathParams(getRequestPathParams(path_items));
			request.setUrlParams(getVariables(getVariableSequence(request_info[1])));
			continue;
		}

		// Reading headers and params
		int header_separator = body[i].indexOf(':');
		int param_separator = body[i].indexOf('=');
		if ((header_separator == -1) && (param_separator == -1) && (body[i].length() > 0))
		{
			writeResponse(HttpResponse(HttpError{StatusCode::BAD_REQUEST, ContentType::TEXT_HTML, "Malformed element: " + body[i]}));
			return;
		}

		if (header_separator > -1)
		{
			request.addHeader(body[i].left(header_separator).toLower(), body[i].mid(header_separator+1).trimmed());
		}
		else if (param_separator > -1)
		{
			request.setFormUrlEncoded(getVariables(body[i]));
		}
	}

	request.setContentType(ContentType::TEXT_HTML);
	if (request.getHeaders().contains("accept"))
	{
		if (HttpProcessor::getContentTypeFromString(request.getHeaders()["accept"]) == ContentType::APPLICATION_JSON)
		{
			request.setContentType(ContentType::APPLICATION_JSON);
		}
	}

	// Executing in a separate thread
	WorkerThread *workerThread = new WorkerThread(request);
	connect(workerThread, &WorkerThread::dataChunkReady, this, &RequestHandler::handleDataChunk);
	connect(workerThread, &WorkerThread::resultReady, this, &RequestHandler::handleResults);
	connect(workerThread, &WorkerThread::finished, workerThread, &QObject::deleteLater);
	workerThread->start();
}

void RequestHandler::dataReceived()
{	
	readRequest(getRequestBody());
}

void RequestHandler::writeResponse(HttpResponse response)
{
	socket_->write(response.getHeaders());
	socket_->write(response.getPayload());
	socket_->flush();
	socket_->close();
	socket_->deleteLater();
}

bool RequestHandler::hasEndOfLineCharsOnly(QByteArray line)
{
	if (line == QByteArrayLiteral("\r\n") || line == QByteArrayLiteral("\n"))
	{
		return true;
	}
	return false;
}

void RequestHandler::handleResults(const HttpResponse &response)
{
	writeResponse(response);
}

void RequestHandler::handleDataChunk(const QByteArray& data)
{
	QString string_data = QString(data).trimmed();
	if (string_data != "%end%") socket_->write(data);

	if (string_data == "%end%")
	{

		if (!socket_->bytesToWrite())
		{
			qDebug() << "Closing the socket";
			socket_->close();
			socket_->deleteLater();
		}
		else {
			qDebug() << "Cannot close the socket, the server is still sending the data";
		}
	}
}

