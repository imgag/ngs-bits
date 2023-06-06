#include "ApiCaller.h"
#include "Settings.h"
#include "Exceptions.h"
#include "ClientHelper.h"
#include "LoginManager.h"
#include "Log.h"

#include <QNetworkProxy>

ApiCaller::ApiCaller()
{
}

QByteArray ApiCaller::get(QString api_path, RequestUrlParams url_params, HttpHeaders headers, bool needs_user_token, bool needs_db_token, bool rethrow_excpetion)
{
	try
	{	
		if (needs_user_token) addUserTokenIfExists(url_params);
		if (needs_db_token) addDbTokenIfExists(url_params);

		return HttpRequestHandler(QNetworkProxy(QNetworkProxy::NoProxy)).get(ClientHelper::serverApiUrl() + api_path + QUrl(url_params.asString()).toEncoded(), headers);
	}
	catch (Exception& e)
	{
		QString message = "API GET call to \"" + ClientHelper::serverApiUrl() + api_path + "\" failed: " + e.message();
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

		return HttpRequestHandler(QNetworkProxy(QNetworkProxy::NoProxy)).post(ClientHelper::serverApiUrl() + api_path + QUrl(url_params.asString()).toEncoded(), data, headers);
	}
	catch (Exception& e)
	{				
		Log::error("API POST call to \"" + ClientHelper::serverApiUrl() + api_path + "\" failed: " + e.message());
		if (rethrow_excpetion) THROW(Exception, e.message());
	}

	return QByteArray{};
}

void ApiCaller::addUserTokenIfExists(RequestUrlParams& params)
{
	try
	{
		params.insert("token", LoginManager::userToken().toUtf8());
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
		params.insert("dbtoken", LoginManager::dbToken().toUtf8());
	}
	catch (ProgrammingException& e)
	{
		Log::error("Could not add database token: " + e.message());
	}
}
