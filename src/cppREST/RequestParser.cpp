#include "RequestParser.h"
#include <QRegularExpression>
#include "Settings.h"
#include "Log.h"
#include "Exceptions.h"
#include "HttpUtils.h"
#include "ServerHelper.h"

RequestParser::RequestParser()
{
}

HttpRequest RequestParser::parse(QByteArray *request) const
{
    HttpRequest parsed_request;

    if (Settings::boolean("show_raw_request", true))
    {
        Log::info(*request);
    }

	QList<QByteArray> body = getRawRequestHeaders(*request);	
	for (int i = 0; i < body.count(); ++i)
	{
		// First line with method type and URL
		if (i == 0)
		{
			QList<QByteArray> request_info = body[i].split(' ');
			if (request_info.length() < 2)
			{
				THROW(Exception, "Cannot process the request. Probably the URL is missing or incorrect");
				return parsed_request;
			}
			parsed_request.setMethod(inferRequestMethod(request_info[0].toUpper()));

			QList<QString> path_items = QString(request_info[1]).split('/');

			parsed_request.setPrefix(getRequestPrefix(path_items));
			parsed_request.setPath(getRequestPath(path_items));
			parsed_request.setPathItems(getRequestPathParams(path_items));
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

	parsed_request.setBody(getRequestBody(*request).trimmed());	
	parsed_request.setContentType(ContentType::TEXT_HTML); // default type, if the header is not found

	if (parsed_request.getHeaders().contains("content-type"))
	{
		QList<QString> headers = parsed_request.getHeaders()["content-type"];
		if (headers.isEmpty()) return parsed_request;

		QList<QString> content_type_header_list = headers[0].split(";");
		QString content_type;
		if (content_type_header_list.count() > 1)
        {
            content_type = content_type_header_list[0];
			QString boundary = "--" + content_type_header_list[1].trimmed().replace("boundary=", "", Qt::CaseInsensitive).replace("\"", "");

			// Parse mutipart form data request
			if ((HttpUtils::getContentTypeFromString(content_type) == ContentType::MULTIPART_FORM_DATA) && (!boundary.isEmpty()))
			{
				QByteArray form_body = parsed_request.getBody();				
				QList<QByteArray> multipart_list;

				// Getting starting positions of all boundaries from the multipart request body
				QList<int> boundary_start_positions = getBoundaryStartPositions(form_body, boundary);

				for (int p = 0; p < boundary_start_positions.count(); p++)
				{
					int boundary_end_position;
					if (p<boundary_start_positions.count()-1)
					{
						boundary_end_position = boundary_start_positions[p+1] - (boundary_start_positions[p]+boundary.length());
					}
					else
					{
						boundary_end_position = (boundary_start_positions[p]+boundary.length()) - (form_body.length());
					}
					QByteArray multipart_item = form_body.mid(boundary_start_positions[p]+boundary.length(), boundary_end_position);                  

					if (multipart_item!="--")
                    {

                        if (!multipart_item.toLower().contains("content-type"))
						{
							// Parse form paramenters (i.e. form fields)
							int name_pos = multipart_item.toLower().indexOf("name");
							multipart_item = multipart_item.mid(name_pos, multipart_item.length() - name_pos);

							// Get parameter value
							QString param_value;
							int value_start = multipart_item.indexOf(empty_line.toUtf8());
							if (value_start > -1)
							{
								param_value = multipart_item.mid(value_start+empty_line.length(), multipart_item.length()-(value_start+empty_line.length())).trimmed();
							}

							// Get parameter name
							QString param_key;
							int key_start = multipart_item.indexOf("=");
							if (key_start > -1)
							{
								param_key = multipart_item.mid(key_start, value_start - key_start).trimmed();
								if (param_key.startsWith("=\"")) param_key = param_key.remove(0,2);
								if (param_key.endsWith("\"")) param_key = param_key.remove(param_key.length()-1,1);
							}

							if ((!param_key.isEmpty()) && (!param_value.isEmpty())) parsed_request.addFormDataParam(param_key, param_value);
						}
						else
						{
                            // Parse file related content
                            if (parsed_request.getMultipartFileName().isEmpty())
                            {
                                parsed_request.setMultipartFileName(getMultipartFileName(multipart_item));
                                parsed_request.setMultipartFileContent(getMultipartFileContent(multipart_item));
                            }
						}
						multipart_list.append(multipart_item);
					}
				}
			}
		}
		else
		{
			content_type = headers[0];
		}
		parsed_request.setContentType(HttpUtils::getContentTypeFromString(content_type));
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
	QList<QByteArray> request_items = input.split(end_of_line);
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
	QList<QByteArray> request_items = input.split(end_of_line);
	bool passed_headers = false;
	for (int i = 0; i < request_items.count(); ++i)
	{
		if (request_items[i].trimmed().isEmpty()) passed_headers = true;
		if (passed_headers)
		{
			output.append(request_items[i]);
			output.append(end_of_line);
		}
	}

	return output.trimmed();
}

QList<QByteArray> RequestParser::getKeyValuePair(const QByteArray& input) const
{
	QList<QByteArray> result;
	if (input.indexOf('=')>-1) result = input.split('=');

	return result;
}

QMap<QString, QString> RequestParser::getVariables(const QByteArray& input) const
{
	QMap<QString, QString> url_vars {};
	QList<QByteArray> var_list = input.split('&');

	for (int i = 0; i < var_list.count(); ++i)
	{
		QList<QByteArray> pair = getKeyValuePair(var_list[i]);
		if (pair.length()==2) url_vars.insert(pair[0], pair[1]);
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
				QString current_item = path_items[p].trimmed();
				int param_separator = current_item.indexOf("?");
				if (param_separator > -1)
				{
					current_item = current_item.left(param_separator);
				}
				if (current_item.length() == 0) continue;
				params.append(current_item);
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

	THROW(ArgumentException, "Unsupported request method");
}

QList<int> RequestParser::getBoundaryStartPositions(const QByteArray& form, const QString& boundary) const
{
    QList<int> boundary_start_positions;
    QRegularExpression boundary_reg_exp(QRegularExpression::escape(boundary));
	QRegularExpressionMatchIterator i = boundary_reg_exp.globalMatch(form);
	while (i.hasNext())
	{
		QRegularExpressionMatch match = i.next();
		if (match.hasMatch()) boundary_start_positions.append(match.capturedStart());
	}
	return boundary_start_positions;
}

QString RequestParser::getMultipartFileName(const QByteArray& multipart_item) const
{
	QRegularExpression file_name_fragment_regexp("filename=\"[^<]+\"", QRegularExpression::CaseInsensitiveOption);
	QRegularExpressionMatch file_name_fragment_match = file_name_fragment_regexp.match(multipart_item);
	if (file_name_fragment_match.hasMatch())
	{
		QString multipart_filename = file_name_fragment_match.captured(0);
		multipart_filename = multipart_filename.replace("filename=\"", "", Qt::CaseInsensitive);
		multipart_filename = multipart_filename.remove(multipart_filename.length()-1,1);
		return multipart_filename;
	}
	return "";
}

QByteArray RequestParser::getMultipartFileContent(QByteArray& multipart_item) const
{
	int content_start = multipart_item.indexOf(empty_line.toUtf8());
	if (content_start > -1)
	{
		// Remove Headers before the content
		multipart_item.remove(0, content_start+empty_line.length());
		// Remove \r\n characters before the last boundary
		if (multipart_item.length() > 2) multipart_item.remove(multipart_item.length()-2, 2);
		return multipart_item;
	}
	return QByteArray();
}
