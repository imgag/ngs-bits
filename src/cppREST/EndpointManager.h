#ifndef ENDPOINTMANAGER_H
#define ENDPOINTMANAGER_H

#include "cppREST_global.h"
#include <QDebug>
#include <QFile>
#include "ServerHelper.h"
#include "HtmlEngine.h"
#include "HttpRequest.h"
#include "HttpResponse.h"
#include "WebExceptions.h"

struct CPPRESTSHARED_EXPORT ParamProps
{
	enum ParamType
	{
		INTEGER,
		STRING,
		ENUM,
		UNKNOWN
	};
	enum ParamCategory
	{
		PATH_PARAM, // http://url/{param}
		GET_URL_PARAM, // http://url?var=val
		POST_URL_ENCODED, // application/x-www-form-urlencoded
		POST_FORM_DATA // multipart/form-data
	};

	ParamType type;
	ParamCategory category;
	bool is_optional;
	QString comment;

	bool operator==(const ParamProps& p) const
	{
		return type==p.type && category==p.category && is_optional==p.is_optional;
	}	
};

struct CPPRESTSHARED_EXPORT Endpoint
{
	QString url;
	QMap<QString, ParamProps> params;
	RequestMethod method;
	ContentType return_type;
	QString comment;
	HttpResponse (*action_func)(HttpRequest request);

	bool operator==(const Endpoint& e) const
	{
		return url==e.url && params==e.params && method==e.method && return_type==e.return_type;
	}
};

class CPPRESTSHARED_EXPORT EndpointManager
{

public:
	static ParamProps::ParamType getParamTypeFromString(QString in);	
	static void validateInputData(Endpoint* current_endpoint, HttpRequest request);
	static void appendEndpoint(Endpoint new_endpoint);	
	static QString generateGlobalHelp();
	static QString generateEntityHelp(QString path, RequestMethod method);

	static Endpoint getEndpointEntity(QString url, RequestMethod method);

protected:
	EndpointManager();

private:	
	static EndpointManager& instance();
	static bool isParamTypeValid(QString param, ParamProps::ParamType type);

	static QString getEndpointHelpTemplate(QList<Endpoint>* endpoint_list);
	QList<Endpoint> endpoint_registry_;
};

#endif // ENDPOINTMANAGER_H
