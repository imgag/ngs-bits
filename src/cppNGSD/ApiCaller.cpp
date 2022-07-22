#include "ApiCaller.h"
#include "Exceptions.h"
#include "NGSHelper.h"
#include "LoginManager.h"

ApiCaller::ApiCaller()
{

}

QByteArray ApiCaller::get(QString api_path, RequestUrlParams url_params, HttpHeaders headers, bool needs_user_token, bool needs_db_token, bool rethrow_excpetion)
{
	try
	{	
		if (needs_user_token) addUserTokenIfExists(url_params);
		if (needs_db_token) addDbTokenIfExists(url_params);

		return HttpRequestHandler(HttpRequestHandler::NONE).get(NGSHelper::serverApiUrl() + api_path + url_params.asString(), headers);
	}
	catch (Exception& e)
	{
		QString message = "API GET call to \"" + NGSHelper::serverApiUrl() + api_path + "\" failed: " + e.message();
		Log::error(message);
		if (rethrow_excpetion) THROW(Exception, message);
	}

	return QByteArray{};
}

QByteArray ApiCaller::post(QString api_path, RequestUrlParams url_params, HttpHeaders headers, const QByteArray& data, bool needs_user_token, bool needs_db_token, bool rethrow_excpetion)
{
	try
	{	
		headers.insert("Content-Type", "application/x-www-form-urlencoded");

		if (needs_user_token) addUserTokenIfExists(url_params);
		if (needs_db_token) addDbTokenIfExists(url_params);

		return HttpRequestHandler(HttpRequestHandler::NONE).post(NGSHelper::serverApiUrl() + api_path + url_params.asString(), data, headers);
	}
	catch (Exception& e)
	{				
		Log::error("API POST call to \"" + NGSHelper::serverApiUrl() + api_path + "\" failed: " + e.message());
		if (rethrow_excpetion) THROW(Exception, e.message());
	}

	return QByteArray{};
}

void ApiCaller::addUserTokenIfExists(RequestUrlParams& params)
{
	try
	{
		params.insert("token", LoginManager::userToken().toLocal8Bit());
	}
	catch (ProgrammingException& e)
	{
		Log::error("Could not add user token: " + e.message());
	}
}

void ApiCaller::addDbTokenIfExists(RequestUrlParams& params)
{
	try
	{
		params.insert("dbtoken", LoginManager::dbToken().toLocal8Bit());
	}
	catch (ProgrammingException& e)
	{
		Log::error("Could not add database token: " + e.message());
	}
}
