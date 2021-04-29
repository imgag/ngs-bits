#include "EndpointManager.h"

EndpointManager::EndpointManager()
{
}

ParamProps::ParamType EndpointManager::getParamTypeFromString(QString in)
{
	if (in.toLower() == "string") return ParamProps::ParamType::STRING;
	if ((in.toLower() == "int") || (in.toLower() == "integer")) return ParamProps::ParamType::INTEGER;

	return ParamProps::ParamType::UNKNOWN;
}

void EndpointManager::validateInputData(Endpoint* current_endpoint, HttpRequest request)
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
				if (!isParamTypeValid(request.getFormUrlEncoded()[i.key()], current_endpoint->params[i.key()].type))
				{
					THROW(ArgumentException, i.key() + " x-www-form-urlencoded parameter has an invalid type");
				}
			}
		}

		if(i.value().category == ParamProps::ParamCategory::GET_URL_PARAM)
		{
			if (request.getUrlParams().contains(i.key()))
			{
				is_found = true;				
				if (!isParamTypeValid(request.getUrlParams()[i.key()], current_endpoint->params[i.key()].type))
				{
					THROW(ArgumentException, i.key() + " parameter inside URL has an invalid type");
				}
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
	if (!instance().endpoint_registry_.contains(new_endpoint))
	{
		instance().endpoint_registry_.append(new_endpoint);
	}
}

EndpointManager& EndpointManager::instance()
{
	static EndpointManager endpoint_factory;
	return endpoint_factory;
}

bool EndpointManager::isParamTypeValid(QString param, ParamProps::ParamType type)
{
	switch (type)
	{
		case ParamProps::ParamType::STRING: return true;
		case ParamProps::ParamType::INTEGER: return ServerHelper::canConvertToInt(param);

		default: return false;
	}
}

Endpoint EndpointManager::getEndpointEntity(QString url, RequestMethod method)
{
	for (int i = 0; i < instance().endpoint_registry_.count(); ++i)
	{
		if ((instance().endpoint_registry_[i].url.toLower() == url.toLower()) &&
			(instance().endpoint_registry_[i].method == method))
		{
			return instance().endpoint_registry_[i];
		}
	}

	return Endpoint{};
}

QList<Endpoint>* EndpointManager::getEndpointEntities()
{
	return &instance().endpoint_registry_;
}
