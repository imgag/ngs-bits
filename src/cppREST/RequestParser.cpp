#include "RequestParser.h"

RequestParser::RequestParser()
{
}

HttpRequest RequestParser::parse(QByteArray *request) const
{
	HttpRequest parsed_request;
	QList<QByteArray> body = getRawRequestHeaders(*request);

	for (int i = 0; i < body.count(); ++i)
	{
		// First line with method type and URL
		if (i == 0)
		{
			QList<QByteArray> request_info = body[i].split(' ');
			if (request_info.length() < 2)
			{
				THROW(Exception, "Cannot process the request. It is possible the URL is missing or incorrect");
				return parsed_request;
			}
			parsed_request.setMethod(inferRequestMethod(request_info[0].toUpper()));

			QList<QString> path_items = QString(request_info[1]).split('/');
			parsed_request.setPrefix(getRequestPrefix(path_items));
			parsed_request.setPath(getRequestPath(path_items));
			parsed_request.setPathParams(getRequestPathParams(path_items));
			parsed_request.setUrlParams(getVariables(getVariableSequence(request_info[1])));
			continue;
		}

		// Reading headers and params
		int header_separator = body[i].indexOf(':');
		int param_separator = body[i].indexOf('=');
		if ((header_separator == -1) && (param_separator == -1) && (body[i].length() > 0))
		{
			THROW(Exception, "Malformed element: " + body[i]);
			return parsed_request;
		}

		if (header_separator > -1)
		{
			QList<QByteArray> header_values = body[i].mid(header_separator+1).trimmed().split(',');
			for (int h = 0; h < header_values.count(); ++h)
			{
				if (header_values[h].trimmed().length() == 0) continue;
				parsed_request.addHeader(body[i].left(header_separator).toLower(), header_values[h].trimmed());
			}
		}
	}

	qDebug() << getRequestBody(*request).trimmed();

	parsed_request.setBody(getRequestBody(*request).trimmed());
	parsed_request.setContentType(ContentType::TEXT_HTML);
//	if (parsed_request.getHeaders().contains("accept"))
//	{
//		QList<QString> headers = parsed_request.getHeaders()["accept"];
//		if (headers.isEmpty()) return parsed_request;
//		if (HttpProcessor::getContentTypeFromString(headers[0]) == ContentType::APPLICATION_JSON)
//		{
//			parsed_request.setContentType(ContentType::APPLICATION_JSON);
//		}
//	}

	if (parsed_request.getHeaders().contains("content-type"))
	{
		QList<QString> headers = parsed_request.getHeaders()["content-type"];
		if (headers.isEmpty()) return parsed_request;
		parsed_request.setContentType(HttpProcessor::getContentTypeFromString(headers[0]));
	}

	if (parsed_request.getContentType() == ContentType::APPLICATION_X_WWW_FORM_URLENCODED)
	{
		parsed_request.setFormUrlEncoded(getVariables(parsed_request.getBody()));
	}

	return parsed_request;
}

QList<QByteArray> RequestParser::getRawRequestHeaders(const QByteArray& input) const
{
	QList<QByteArray> output;
	QList<QByteArray> request_items = input.split('\n');
	for (int i = 0; i < request_items.count(); ++i)
	{
		if (request_items.value(i).trimmed().isEmpty()) return output;
		output.append(request_items.value(i).trimmed());
	}
	return output;
}

QByteArray RequestParser::getRequestBody(const QByteArray& input) const
{
	QByteArray output;
	QList<QByteArray> request_items = input.split('\n');
	bool passed_headers = false;
	for (int i = 0; i < request_items.count(); ++i)
	{
		if (request_items[i].trimmed().isEmpty()) passed_headers = true;
		if (passed_headers)
		{
			output.append(request_items[i]);
			output.append('\n');
		}
	}

	return output.trimmed();
}

QList<QByteArray> RequestParser::getKeyValuePair(const QByteArray& input) const
{
	QList<QByteArray> result;

	if (input.indexOf('=')>-1)
	{
		result = input.split('=');
	}

	return result;
}

QMap<QString, QString> RequestParser::getVariables(const QByteArray& input) const
{
	QMap<QString, QString> url_vars {};
	QList<QByteArray> var_list = input.split('&');
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

QByteArray RequestParser::getVariableSequence(const QByteArray& url) const
{
	QByteArray var_string;
	if (url.indexOf('?') == -1) return var_string;
	return url.split('?')[1];
}

QString RequestParser::getRequestPrefix(const QList<QString>& path_items) const
{
	if (path_items.count()>1)
	{
		return ServerHelper::getUrlWithoutParams(path_items[1]);
	}
	return "";
}

QString RequestParser::getRequestPath(const QList<QString>& path_items) const
{
	if (path_items.count()>2)
	{
		return ServerHelper::getUrlWithoutParams(path_items[2]);
	}
	return "";
}

QList<QString> RequestParser::getRequestPathParams(const QList<QString>& path_items) const
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

RequestMethod RequestParser::inferRequestMethod(const QByteArray& input) const
{
	if (input.toUpper() == QByteArrayLiteral("GET"))
	{
		return RequestMethod::GET;
	}
	if (input.toUpper() == QByteArrayLiteral("POST"))
	{
		return RequestMethod::POST;
	}
	if (input.toUpper() == QByteArrayLiteral("DELETE"))
	{
		return RequestMethod::DELETE;
	}
	if (input.toUpper() == QByteArrayLiteral("PUT"))
	{
		return RequestMethod::PUT;
	}
	if (input.toUpper() == QByteArrayLiteral("PATCH"))
	{
		return RequestMethod::PATCH;
	}
	if (input.toUpper() == QByteArrayLiteral("HEAD"))
	{
		return RequestMethod::HEAD;
	}

	THROW(ArgumentException, "Incorrect request method");
}
