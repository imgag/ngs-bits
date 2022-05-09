#ifndef ENDPOINTMANAGER_H
#define ENDPOINTMANAGER_H

#include "cppREST_global.h"
#include <QDebug>
#include <QFile>
#include "ServerHelper.h"
#include "HttpRequest.h"
#include "HttpResponse.h"
#include "SessionManager.h"
#include "NGSD.h"

struct CPPRESTSHARED_EXPORT ParamProps
{
	enum ParamCategory
	{
		PATH_PARAM, // http://url/{param}
		GET_URL_PARAM, // http://url?var=val
		POST_URL_ENCODED, // application/x-www-form-urlencoded
		POST_FORM_DATA, // multipart/form-data, not implemented yet
		POST_OCTET_STREAM // application/octet-stream
	};
	ParamCategory category;
	bool is_optional;
	QString comment;

	bool operator==(const ParamProps& p) const
	{
		return category==p.category && is_optional==p.is_optional;
	}	
};

typedef enum
{
	NONE,
	HTTP_BASIC_AUTH,
	USER_TOKEN,
	DB_TOKEN
} AuthType;

struct CPPRESTSHARED_EXPORT Endpoint
{
	QString url;
	QMap<QString, ParamProps> params;
	RequestMethod method;
	ContentType return_type;
	AuthType authentication_type;
	QString comment;
	HttpResponse (*action_func)(const HttpRequest& request);

	bool operator==(const Endpoint& e) const
	{
		return url==e.url && params==e.params && method==e.method && return_type==e.return_type;
	}
};

class CPPRESTSHARED_EXPORT EndpointManager
{

public:
	static HttpResponse getBasicHttpAuthStatus(HttpRequest request);

	/// Checks if a valid token has been provided
	static bool isAuthorizedWithToken(const HttpRequest& request);
	/// Checks if the secure token is valid and not expired
	static HttpResponse getUserTokenAuthStatus(const HttpRequest& request);
	/// Check if GSvar toje is valid
	static HttpResponse getDbTokenAuthStatus(const HttpRequest& request);

	static void validateInputData(Endpoint* current_endpoint, const HttpRequest& request);
	static void appendEndpoint(Endpoint new_endpoint);	
	static Endpoint getEndpointByUrlAndMethod(const QString& url, const RequestMethod& method);
	static QList<Endpoint> getEndpointsByUrl(const QString& url);
	static QList<Endpoint> getEndpointEntities();

protected:
	EndpointManager();

private:	
	static EndpointManager& instance();
	QList<Endpoint> endpoint_list_;
};

#endif // ENDPOINTMANAGER_H
