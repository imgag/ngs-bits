#include "WorkerThread.h"

WorkerThread::WorkerThread(HttpRequest request)
	: request_(request)
{
}

void WorkerThread::run()
{	
	QByteArray body {};
	qInfo().noquote() << request_.methodAsString().toUpper() << "/" << request_.getPath() << request_.getRemoteAddress().toLatin1().data();

	Endpoint current_endpoint = EndpointManager::getEndpointEntity(request_.getPath(), request_.getMethod());
	if (current_endpoint.action_func == nullptr)
	{
		emit resultReady(HttpResponse(HttpError{StatusCode::BAD_REQUEST, request_.getContentType(), "This action cannot be processed"}));
		return;
	}

	try
	{
		EndpointManager::validateInputData(&current_endpoint, request_);
	}
	catch (ArgumentException& e)
	{
		emit resultReady(HttpResponse(HttpError{StatusCode::BAD_REQUEST, request_.getContentType(), e.message()}));
		return;
	}

	qDebug() << "Requested:" << current_endpoint.comment.toLatin1().data();

	endpoint_action_ = current_endpoint.action_func;
	HttpResponse response = (*endpoint_action_)(request_);

	if (response.isStream())
	{
		emit dataChunkReady(response.getHeaders());
		QFile streamed_file(response.getFilename());
		if (!streamed_file.open(QFile::ReadOnly | QIODevice::Text))
		{
			THROW(FileAccessException, "Could not open file for reading");
		}

		QTextStream stream(&streamed_file);
		while(!stream.atEnd())
		{
			QString line = stream.readLine().append("\n");
			emit dataChunkReady(intToHex(line.size()).toLocal8Bit()+"\r\n");
			emit dataChunkReady(line.toLocal8Bit()+"\r\n");
		}
		streamed_file.close();

		emit dataChunkReady("0\r\n");
		emit dataChunkReady("\r\n");
		return;
	}
	else if (!response.getPayload().isNull())
	{
		emit resultReady(response);
		return;
	}

	emit resultReady(HttpResponse(HttpError{StatusCode::NOT_FOUND, request_.getContentType(), "This page does not exist. Check the URL and try again"}));
}

QString WorkerThread::intToHex(const int &in)
{
	return QString("%1").arg(in, 10, 16, QLatin1Char('0')).toUpper();
}
