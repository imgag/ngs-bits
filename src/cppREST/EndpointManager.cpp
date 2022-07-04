#include "EndpointManager.h"
#include "ToolBase.h"

EndpointManager::EndpointManager()
{
}

HttpResponse EndpointManager::getBasicHttpAuthStatus(HttpRequest request)
{
	qDebug() << "Basic HTTP authentication";
	QString auth_header = request.getHeaderByName("Authorization").length() > 0 ? request.getHeaderByName("Authorization")[0] : "";
	if (auth_header.isEmpty())
	{
		return HttpResponse(ResponseStatus::UNAUTHORIZED, HttpProcessor::detectErrorContentType(request.getHeaderByName("User-Agent")), "You are in a protected area. Please provide your credentials");
	}

	if (auth_header.split(" ").size() < 2)
	{
		return HttpResponse(ResponseStatus::BAD_REQUEST, HttpProcessor::detectErrorContentType(request.getHeaderByName("User-Agent")), "Could not parse basic authentication headers");
	}

	auth_header = auth_header.split(" ").takeLast().trimmed();
	QByteArray auth_header_decoded = QByteArray::fromBase64(auth_header.toLatin1());
	int separator_pos = auth_header_decoded.indexOf(":");

	if (separator_pos == -1)
	{
		return HttpResponse(ResponseStatus::BAD_REQUEST, HttpProcessor::detectErrorContentType(request.getHeaderByName("User-Agent")), "Could not retrieve the credentials");
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
		return HttpResponse(ResponseStatus::BAD_REQUEST, HttpProcessor::detectErrorContentType(request.getHeaderByName("User-Agent")), "Database error: " + e.message());
	}

	if (!message.isEmpty())
	{
		return HttpResponse(ResponseStatus::UNAUTHORIZED, HttpProcessor::detectErrorContentType(request.getHeaderByName("User-Agent")), "Invalid user credentials");
	}

	return HttpResponse(ResponseStatus::OK, request.getContentType(), "Successful authorization");
}


bool EndpointManager::isAuthorizedWithToken(const HttpRequest& request)
{
	if (request.getUrlParams().contains("token"))
	{
		qDebug() << "User token from URL" << request.getUrlParams()["token"];
		return SessionManager::isTokenReal(request.getUrlParams()["token"]);
	}
	if (request.getFormUrlEncoded().contains("token"))
	{
		qDebug() << "User token from Form" << request.getFormUrlEncoded()["token"];
		return SessionManager::isTokenReal(request.getFormUrlEncoded()["token"]);
	}
	if (request.getFormUrlEncoded().contains("dbtoken"))
	{
		qDebug() << "Database token from Form" << request.getFormUrlEncoded()["dbtoken"];
		if (!SessionManager::getSessionBySecureToken(request.getFormUrlEncoded()["dbtoken"]).is_for_db_only) return false;
		return SessionManager::isTokenReal(request.getFormUrlEncoded()["dbtoken"]);
	}

	qDebug() << "Invalid token";
	return false;
}

HttpResponse EndpointManager::getUserTokenAuthStatus(const HttpRequest& request)
{
	qDebug() << "Check user token status";
	if (!isAuthorizedWithToken(request))
	{
		return HttpResponse(ResponseStatus::FORBIDDEN, HttpProcessor::detectErrorContentType(request.getHeaderByName("User-Agent")), "You are not authorized with a valid user token");
	}

	QString token;
	if (request.getUrlParams().contains("token")) token = request.getUrlParams()["token"];
	if (request.getFormUrlEncoded().contains("token")) token = request.getFormUrlEncoded()["token"];
	if (SessionManager::isSessionExpired(token))
	{
		return HttpResponse(ResponseStatus::REQUEST_TIMEOUT, request.getContentType(), "Secure token has expired");
	}

	return HttpResponse(ResponseStatus::OK, request.getContentType(), "OK");
}

HttpResponse EndpointManager::getDbTokenAuthStatus(const HttpRequest& request)
{
	qDebug() << "Check db token status";
	if (!isAuthorizedWithToken(request))
	{
		return HttpResponse(ResponseStatus::FORBIDDEN, HttpProcessor::detectErrorContentType(request.getHeaderByName("User-Agent")), "You are not authorized with a valid database token");
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

		if (i.value().category == ParamProps::ParamCategory::POST_URL_ENCODED)
		{
			if (request.getFormUrlEncoded().contains(i.key()))
			{
				is_found = true;
			}
		}

		if (i.value().category == ParamProps::ParamCategory::GET_URL_PARAM)
		{
			if (request.getUrlParams().contains(i.key()))
			{
				is_found = true;
			}
		}

		if (i.value().category == ParamProps::ParamCategory::PATH_PARAM)
		{
			if (request.getPathItems().size()>0)
			{
				is_found = true;
			}
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

EndpointManager& EndpointManager::instance()
{
	static EndpointManager endpoint_factory;
	return endpoint_factory;
}
