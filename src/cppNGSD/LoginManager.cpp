#include "LoginManager.h"
#include "NGSD.h"
#include "ToolBase.h"
#include "Log.h"
#include "ClientHelper.h"
#include <QJsonDocument>
#include <QNetworkProxy>

LoginManager::LoginManager()
	: user_login_()
	, user_name_()
	, user_id_(-1)
	, user_token_()
	, user_password_()
	, db_token_()
	, ngsd_host_name_()
	, ngsd_port_()
	, ngsd_name_()
	, ngsd_user_()
	, ngsd_password_()
	, genlab_mssql_()
	, genlab_host_()
	, genlab_port_()
	, genlab_name_()
	, genlab_user_()
	, genlab_password_()
{
}

LoginManager& LoginManager::instance()
{
	static LoginManager manager;
	return manager;
}

QByteArray LoginManager::sendPostApiRequest(QString path, QString content, HttpHeaders add_headers)
{
    try
	{
        return HttpRequestHandler(QNetworkProxy(QNetworkProxy::NoProxy)).post(ClientHelper::serverApiUrl() + path, content.toUtf8(), add_headers).body;
	}
    catch (HttpException& e)
	{
		Log::error("Login manager encountered an error while sending POST request: " + e.message());
	}

	return QByteArray{};
}

QByteArray LoginManager::sendGetApiRequest(QString path, HttpHeaders add_headers)
{
    QByteArray reply;
    try
	{
        reply = HttpRequestHandler(QNetworkProxy(QNetworkProxy::NoProxy)).get(ClientHelper::serverApiUrl() + path, add_headers).body;
	}
    catch (HttpException& e)
	{
		Log::error("Login manager encountered an error while sending GET request: " + e.message());
        reply = e.message().toUtf8();
    }
    return reply;
}

void LoginManager::setAllTokens(const QString& user, const QString& password)
{
    LoginManager& manager = instance();
    if (ClientHelper::isClientServerMode())
    {
        HttpHeaders add_headers;
        add_headers.insert("Accept", "text/plain");
        add_headers.insert("Content-type", "application/x-www-form-urlencoded");
        manager.user_token_ = sendPostApiRequest("login", "name="+user+"&password="+password, add_headers);
        manager.db_token_ = sendPostApiRequest("db_token", "token="+manager.user_token_, add_headers);
        QByteArray ngsd_credentials = sendPostApiRequest("ngsd_credentials", "dbtoken="+manager.db_token_+"&secret="+QString::number(ToolBase::encryptionKey("encryption helper"), 16), add_headers);
        QJsonDocument ngsd_json = QJsonDocument::fromJson(ngsd_credentials);

        Log::info("manager.user_token_ = " + manager.user_token_);
        if (ngsd_json.isObject())
        {
            manager.ngsd_host_name_ = ngsd_json.object().value("ngsd_host").toString();
            manager.ngsd_port_ = ngsd_json.object().value("ngsd_port").toString().toInt();
            manager.ngsd_name_ = ngsd_json.object().value("ngsd_name").toString();
            manager.ngsd_user_ = ngsd_json.object().value("ngsd_user").toString();
            manager.ngsd_password_ = ngsd_json.object().value("ngsd_pass").toString();
        }

        QByteArray genlab_credentials = sendPostApiRequest("genlab_credentials", "dbtoken="+manager.db_token_+"&secret="+QString::number(ToolBase::encryptionKey("encryption helper"), 16), add_headers);
        QJsonDocument genlab_json = QJsonDocument::fromJson(genlab_credentials);

        if (genlab_json.isObject())
        {
            manager.genlab_mssql_ = genlab_json.object().value("genlab_mssql").toBool();
            manager.genlab_host_ = genlab_json.object().value("genlab_host").toString();
            manager.genlab_port_ = genlab_json.object().value("genlab_port").toInt();
            manager.genlab_name_ = genlab_json.object().value("genlab_name").toString();
            manager.genlab_user_ = genlab_json.object().value("genlab_user").toString();
            manager.genlab_password_ = genlab_json.object().value("genlab_pass").toString();
        }
    }
}

QString LoginManager::userLogin()
{
	return instance().user_login_;
}


QString LoginManager::userName()
{
	return instance().user_name_;
}

int LoginManager::userId()
{
	int id = instance().user_id_;
	if (id==-1) THROW(ProgrammingException, "Cannot use LoginManager::userId() if no user is logged in!");

	return id;
}

QString LoginManager::userIdAsString()
{
	int id = instance().user_id_;
	if (id==-1) THROW(ProgrammingException, "Cannot use LoginManager::userIdAsString() if no user is logged in!");

	return QString::number(id);
}

QString LoginManager::userToken()
{
	QString token = instance().user_token_;
	if (token.isEmpty()) THROW(ProgrammingException, "Cannot use LoginManager::userToken() if no user is logged in!");

	return token;
}

QString LoginManager::userPassword()
{
	QString password = instance().user_password_;
	if (password.isEmpty()) THROW(ProgrammingException, "Cannot use LoginManager::userPassword() if no user is logged in!");

	return password;
}

bool LoginManager::active()
{
	return !instance().user_login_.isEmpty();
}

void LoginManager::login(QString user, QString password, bool test_db)
{
    LoginManager& manager = instance();

    setAllTokens(user, password); // for client-server mode

	NGSD db(test_db);
	manager.user_id_ = db.userId(user, true);
	manager.user_login_ = user;
	manager.user_name_ = db.userName(manager.user_id_);
	manager.user_password_ = password;

	//update last login
	db.getQuery().exec("UPDATE user SET last_login=NOW() WHERE id='" + QString::number(manager.user_id_) + "'");
}

void LoginManager::renewLogin()
{
	LoginManager& manager = instance();

	QString user_login;
	QString user_password;
	try
	{
		user_login = manager.userLogin();
		user_password = manager.userPassword();
	}
    catch (...)
    {
		Log::info("The user has not logged in yet. No need to update the credentials");
		return;
	}

    HttpHeaders session_headers;
    session_headers.insert("Accept", "application/json");
    session_headers.insert("Content-type", "application/x-www-form-urlencoded");
    QByteArray session_info = sendGetApiRequest("session?token=" + manager.userToken(), session_headers);
	QJsonDocument session_json = QJsonDocument::fromJson(session_info);

	if (session_json.isObject())
	{
        qint64 login_time = session_json.object().value("login_time").toInt();
        qint64 valid_period = session_json.object().value("valid_period").toInt();
		// request a new token, if the current one is about to expire (30 minutes in advance)
        if ((login_time + valid_period - (0.5 * 3600)) < QDateTime::currentDateTime().toSecsSinceEpoch())
		{
			if ((user_login.isEmpty()) || (user_password.isEmpty())) return;
            setAllTokens(user_login, user_password);
		}
	}
    else if (session_info.contains("expired"))
    {
        // the server could not return session info, the current token is likely expired
        Log::info("Token has expired, request a new one");
        setAllTokens(user_login, user_password);
    }
}

QString LoginManager::dbToken()
{
	QString db_token = instance().db_token_;
	if (db_token.isEmpty()) THROW(ProgrammingException, "Cannot use LoginManager::db_token before logging in!");

	return db_token;
}

void LoginManager::logout()
{
	LoginManager& manager = instance();

    if (ClientHelper::isClientServerMode())
    {
        HttpHeaders add_headers;
        add_headers.insert("Accept", "text/plain");
        add_headers.insert("Content-type", "application/x-www-form-urlencoded");
        sendPostApiRequest("logout", "token="+manager.userToken(), add_headers);
    }

	manager.user_login_.clear();
	manager.user_name_.clear();
	manager.user_id_ = -1;
	manager.user_token_.clear();
	manager.user_name_.clear();	
	manager.user_password_.clear();
	manager.db_token_.clear();
	manager.ngsd_host_name_.clear();
	manager.ngsd_port_ = -1;
	manager.ngsd_name_.clear();
	manager.ngsd_user_.clear();
	manager.ngsd_password_.clear();
	manager.genlab_mssql_ = false;
	manager.genlab_host_.clear();
	manager.genlab_port_ = -1;
	manager.genlab_name_.clear();
	manager.genlab_user_.clear();
	manager.genlab_password_.clear();
}

QString LoginManager::ngsdHostName()
{
	QString ngsd_host_name = instance().ngsd_host_name_;
	if (ngsd_host_name.isEmpty()) THROW(ProgrammingException, "Could not retrieve database credentials: ngsd_host_name");

	return ngsd_host_name;
}

int LoginManager::ngsdPort()
{
	int ngsd_port = instance().ngsd_port_;
	if (ngsd_port <= 0) THROW(ProgrammingException, "Could not retrieve database credentials:: ngsd_port");

	return ngsd_port;
}

QString LoginManager::ngsdName()
{
	QString ngsd_name = instance().ngsd_name_;
	if (ngsd_name.isEmpty()) THROW(ProgrammingException, "Could not retrieve database credentials: ngsd_name");

	return ngsd_name;
}

QString LoginManager::ngsdUser()
{
	QString ngsd_user = instance().ngsd_user_;
	if (ngsd_user.isEmpty()) THROW(ProgrammingException, "Could not retrieve database credentials: ngsd_user");

	return ngsd_user;
}

QString LoginManager::ngsdPassword()
{
	QString ngsd_password = instance().ngsd_password_;
	if (ngsd_password.isEmpty()) THROW(ProgrammingException, "Could not retrieve database credentials: ngsd_password");

	return ngsd_password;
}

bool LoginManager::genlab_mssql()
{
	return instance().genlab_mssql_;
}

QString LoginManager::genlabHost()
{
	QString genlab_host = instance().genlab_host_;
	if (genlab_host.isEmpty()) THROW(ProgrammingException, "Could not retrieve database credentials: genlab_host");

	return genlab_host;
}

int LoginManager::genlabPort()
{
	return instance().genlab_port_;
}

QString LoginManager::genlabName()
{
	QString genlab_name = instance().genlab_name_;
	if (genlab_name.isEmpty()) THROW(ProgrammingException, "Could not retrieve database credentials: genlab_name");

	return genlab_name;
}

QString LoginManager::genlabUser()
{
	QString genlab_user = instance().genlab_user_;
	if (genlab_user.isEmpty()) THROW(ProgrammingException, "Could not retrieve database credentials: genlab_user");

	return genlab_user;
}

QString LoginManager::genlabPassword()
{
	QString genlab_password = instance().genlab_password_;
	if (genlab_password.isEmpty()) THROW(ProgrammingException, "Could not retrieve database credentials: genlab_password");

	return genlab_password;
}

void LoginManager::checkRoleIn(QStringList roles)
{
	//check if user has role
	LoginManager& manager = instance();
	if (!NGSD().userRoleIn(manager.user_login_, roles))
	{
		INFO(AccessDeniedException, "Access denied.\nOnly users with the following roles have access to this functionality: " + roles.join(", ") + ".\nThe user '" + manager.user_login_ + "' has the role '" + NGSD().getUserRole(manager.userId()) + "'!");
	}
}

void LoginManager::checkRoleNotIn(QStringList roles)
{
	NGSD db;

	LoginManager& manager = instance();
	if (db.userRoleIn(manager.user_login_, roles))
	{
		//invert role selection for output
		QStringList roles_db = db.getEnum("user", "user_role");
		roles = roles_db.toSet().subtract(roles.toSet()).toList();

		INFO(AccessDeniedException, "Access denied.\nOnly users with the following roles have access to this functionality: " + roles.join(", ") + ".\nThe user '" + manager.user_login_ + "' has the role '" + NGSD().getUserRole(manager.userId()) + "'!");
	}
}
