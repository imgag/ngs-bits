#include "EndpointManager.h"
#include "ToolBase.h"

EndpointManager::EndpointManager()
{
}

HttpResponse EndpointManager::getBasicHttpAuthStatus(HttpRequest request)
{
	QString auth_header = request.getHeaderByName("Authorization").length() > 0 ? request.getHeaderByName("Authorization")[0] : "";
	if (auth_header.isEmpty())
	{
		return HttpResponse(ResponseStatus::UNAUTHORIZED, HttpUtils::detectErrorContentType(request.getHeaderByName("User-Agent")), "You are in a protected area. Please provide your credentials");
	}

	if (auth_header.split(" ").size() < 2)
	{
		return HttpResponse(ResponseStatus::BAD_REQUEST, HttpUtils::detectErrorContentType(request.getHeaderByName("User-Agent")), "Could not parse basic authentication headers");
	}

	auth_header = auth_header.split(" ").takeLast().trimmed();
	QByteArray auth_header_decoded = QByteArray::fromBase64(auth_header.toUtf8());
	int separator_pos = auth_header_decoded.indexOf(":");

	if (separator_pos == -1)
	{
		return HttpResponse(ResponseStatus::BAD_REQUEST, HttpUtils::detectErrorContentType(request.getHeaderByName("User-Agent")), "Could not retrieve the credentials");
	}

	QString username = auth_header_decoded.mid(0, separator_pos);
	QString password = auth_header_decoded.mid(separator_pos+1, auth_header_decoded.size()-username.size()-1);

	QString message;

	try
	{
		message = NGSD().checkPassword(username, password);
	}
	catch (Exception& e)
	{
		return HttpResponse(ResponseStatus::BAD_REQUEST, HttpUtils::detectErrorContentType(request.getHeaderByName("User-Agent")), "Database error: " + e.message());
	}

	if (!message.isEmpty())
	{
		return HttpResponse(ResponseStatus::UNAUTHORIZED, HttpUtils::detectErrorContentType(request.getHeaderByName("User-Agent")), "Invalid user credentials");
	}

	return HttpResponse(ResponseStatus::OK, request.getContentType(), "Successful authorization");
}

QString EndpointManager::getTokenFromHeader(HttpRequest request)
{
	QList<QString> auth_header = request.getHeaderByName("Authorization");
	if (auth_header.count()>0)
	{
		int sep_pos = auth_header.first().indexOf(' ',0);
		if (sep_pos>-1) return auth_header.first().mid(sep_pos+1, auth_header.first().length()-sep_pos);
	}
	return "";
}

QString EndpointManager::getTokenIfAvailable(HttpRequest request)
{
	QString token = "";
	if (request.getUrlParams().contains("token"))
	{
		token = request.getUrlParams()["token"];
	}
	if (request.getFormUrlEncoded().contains("token"))
	{
		token = request.getFormUrlEncoded()["token"];
	}
	if (request.getFormUrlEncoded().contains("dbtoken"))
	{
		token = request.getFormUrlEncoded()["dbtoken"];
	}
	if (!getTokenFromHeader(request).isEmpty())
	{
		token = getTokenFromHeader(request);
	}
	return token;
}

HttpResponse EndpointManager::getUserTokenAuthStatus(const HttpRequest& request)
{
	if (!SessionManager::isTokenReal(getTokenIfAvailable(request)))
	{
		Log::warn("Invalid or empty secure token has been used");
		return HttpResponse(ResponseStatus::FORBIDDEN, HttpUtils::detectErrorContentType(request.getHeaderByName("User-Agent")), "You are not authorized with a valid user token");
	}

	if (SessionManager::isSessionExpired(getTokenIfAvailable(request)))
	{
		return HttpResponse(ResponseStatus::REQUEST_TIMEOUT, request.getContentType(), "Secure token has expired");
	}

	return HttpResponse(ResponseStatus::OK, request.getContentType(), "OK");
}

HttpResponse EndpointManager::getDbTokenAuthStatus(const HttpRequest& request)
{
	if (!SessionManager::isTokenReal(request.getFormUrlEncoded()["dbtoken"]))
	{
		return HttpResponse(ResponseStatus::FORBIDDEN, HttpUtils::detectErrorContentType(request.getHeaderByName("User-Agent")), "You are not authorized with a valid database token");
	}

	if (SessionManager::isSessionExpired(request.getFormUrlEncoded()["dbtoken"]))
	{
		return HttpResponse(ResponseStatus::REQUEST_TIMEOUT, request.getContentType(), "Database token has expired");
	}

	if (!request.getHeaderByName("User-Agent").contains("GSvar"))
	{
		Log::warn("Unauthorized entity tried to request the database credentials");
		return HttpResponse(ResponseStatus::FORBIDDEN, request.getContentType(), "You are not allowed to request the database credentials. This incident will be reported");
	}

	bool ok = true;
	if (request.getFormUrlEncoded()["secret"].toULongLong(&ok, 16) != ToolBase::encryptionKey("encryption helper"))
	{
		Log::warn("Secret check failed for the database credentials");
		return HttpResponse(ResponseStatus::FORBIDDEN, request.getContentType(), "You are not allowed to request the database credentials. This incident will be reported");
	}

	return HttpResponse(ResponseStatus::OK, request.getContentType(), "OK");
}


void EndpointManager::validateInputData(Endpoint* current_endpoint, const HttpRequest& request)
{	
	QMapIterator<QString, ParamProps> i(current_endpoint->params);
	while (i.hasNext())
	{
		i.next();		
		bool is_found = false;
		if (i.value().category == ParamProps::ParamCategory::POST_OCTET_STREAM)
		{
			if (request.getBody().length()>0)
			{
				is_found = true;
			}
		}

		if ((i.value().category == ParamProps::ParamCategory::POST_URL_ENCODED) || (i.value().category == ParamProps::ParamCategory::ANY))
		{
			if (request.getFormUrlEncoded().contains(i.key()))
			{
				is_found = true;
			}
		}

		if ((i.value().category == ParamProps::ParamCategory::GET_URL_PARAM) || (i.value().category == ParamProps::ParamCategory::ANY))
		{
			if (request.getUrlParams().contains(i.key()))
			{
				is_found = true;
			}
		}

		if ((i.value().category == ParamProps::ParamCategory::PATH_PARAM) || (i.value().category == ParamProps::ParamCategory::ANY))
		{
			if (request.getPathItems().size()>0)
			{
				is_found = true;
			}
		}

		if (!getTokenFromHeader(request).isEmpty())
		{
			is_found = true;
		}

		if ((!i.value().is_optional) && (!is_found))
		{
			THROW(ArgumentException, "Parameter " + i.key() + " is missing");
		}
	}
}

void EndpointManager::appendEndpoint(Endpoint new_endpoint)
{
	if (!instance().endpoint_list_.contains(new_endpoint))
	{
		instance().endpoint_list_.append(new_endpoint);
	}
}

Endpoint EndpointManager::getEndpointByUrlAndMethod(const QString& url, const RequestMethod& method)
{
	for (int i = 0; i < instance().endpoint_list_.count(); ++i)
	{
		if ((instance().endpoint_list_[i].url.toLower() == url.toLower()) &&
			(instance().endpoint_list_[i].method == method))
		{
			return instance().endpoint_list_[i];
		}
	}

	return Endpoint();
}

QList<Endpoint> EndpointManager::getEndpointsByUrl(const QString& url)
{
	QList<Endpoint> results;
	for (int i = 0; i < instance().endpoint_list_.count(); ++i)
	{
		if (instance().endpoint_list_[i].url.toLower() == url.toLower())
		{
			results.append(instance().endpoint_list_[i]);
		}
	}

	return results;
}

QList<Endpoint> EndpointManager::getEndpointEntities()
{
	return instance().endpoint_list_;
}

QString EndpointManager::getEndpointHelpTemplate(QList<Endpoint> endpoint_list)
{
	QString output;
	QTextStream stream(&output);

	stream << HtmlEngine::getPageHeader("API Help Page");
	stream << HtmlEngine::getApiHelpHeader("API Help Page");

	for (int i = 0; i < endpoint_list.count(); ++i)
	{
		QList<QString> param_names {};
		QList<QString> param_desc {};

		QMapIterator<QString, ParamProps> p(endpoint_list[i].params);
		while (p.hasNext()) {
			p.next();
			param_names.append(p.key());
			param_desc.append(p.value().comment);
		}

		HttpRequest request;
		request.setMethod(endpoint_list[i].method);

		stream << HtmlEngine::getApiHelpEntry(
					  endpoint_list[i].url,
					  request.methodAsString(),
					  param_names,
					  param_desc,
					  endpoint_list[i].comment
					);
	}

	stream << HtmlEngine::getPageFooter();

	return output;
}

EndpointManager& EndpointManager::instance()
{
	static EndpointManager endpoint_factory;
	return endpoint_factory;
}
