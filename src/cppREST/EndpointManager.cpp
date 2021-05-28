#include "EndpointManager.h"

EndpointManager::EndpointManager()
{
}

HttpResponse EndpointManager::blockInvalidUsers(HttpRequest request)
{
	QString auth_header = request.getHeaderByName("Authorization");
	if (auth_header.isEmpty())
	{
		return HttpResponse(ResponseStatus::UNAUTHORIZED, HttpProcessor::getContentTypeFromString("text/plain"), "Protected area");
	}

	if (auth_header.split(" ").size() < 2)
	{
		return HttpResponse(ResponseStatus::BAD_REQUEST, request.getContentType(), "Could not parse basic authentication headers");
	}

	auth_header = auth_header.split(" ").takeLast().trimmed();
	QByteArray auth_header_decoded = QByteArray::fromBase64(auth_header.toLatin1());
	int separator_pos = auth_header_decoded.indexOf(":");

	if (separator_pos == -1)
	{
		return HttpResponse(ResponseStatus::FORBIDDEN, request.getContentType(), "Could not retrieve the credentials");
	}

	QString username = auth_header_decoded.mid(0, separator_pos);
	QString password = auth_header_decoded.mid(separator_pos+1, auth_header_decoded.size()-username.size()-1);

	if (!isUserValid(username, password))
	{
		return HttpResponse(ResponseStatus::FORBIDDEN, request.getContentType(), "Invalid user credentials");
	}

	return HttpResponse();
}

void EndpointManager::validateInputData(Endpoint* current_endpoint, const HttpRequest& request)
{	
	QMapIterator<QString, ParamProps> i(current_endpoint->params);
	while (i.hasNext()) {
		i.next();		
		bool is_found = false;

		if(i.value().category == ParamProps::ParamCategory::POST_URL_ENCODED)
		{
			if (request.getFormUrlEncoded().contains(i.key()))
			{
				is_found = true;
			}
		}

		if(i.value().category == ParamProps::ParamCategory::GET_URL_PARAM)
		{
			if (request.getUrlParams().contains(i.key()))
			{
				is_found = true;
			}
		}

		if(i.value().category == ParamProps::ParamCategory::PATH_PARAM)
		{
			if (request.getPathParams().size()>0)
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
		if (instance().endpoint_list_.value(i).url.toLower() == url.toLower())
		{
			results.append(instance().endpoint_list_.value(i));
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

bool EndpointManager::isUserValid(QString& user, QString& password)
{
	try
	{
		NGSD db;
		QString message = db.checkPassword(user, password, true);
		if (message.isEmpty())
		{
			return true;
		}
		else
		{
			return false;
		}

	}
	catch (DatabaseException& e)
	{
		qCritical() << e.message();
	}
	return false;
}
