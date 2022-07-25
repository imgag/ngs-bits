#ifndef APICALLER_H
#define APICALLER_H

#include "cppNGSD_global.h"
#include "HttpRequestHandler.h"

class CPPNGSDSHARED_EXPORT ApiCaller
{
public:
	ApiCaller();
	QByteArray get(QString api_path, RequestUrlParams url_params, HttpHeaders headers, bool needs_user_token = false, bool needs_db_token = false, bool rethrow_excpetion = false);
	QByteArray post(QString api_path, RequestUrlParams url_params, HttpHeaders headers, const QByteArray& data, bool needs_user_token = false, bool needs_db_token = false, bool rethrow_excpetion = false);

protected:
	void addUserTokenIfExists(RequestUrlParams& params);
	void addDbTokenIfExists(RequestUrlParams& params);
};

#endif // APICALLER_H
