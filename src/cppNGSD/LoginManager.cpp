#include "LoginManager.h"
#include "NGSD.h"

LoginManager::LoginManager()
	: user_()
	, user_name_()
	, user_id_(-1)
	, role_()
	, token_()
{
}

LoginManager& LoginManager::instance()
{
	static LoginManager manager;
	return manager;
}

QByteArray LoginManager::sendAuthRequest(QString content, HttpHeaders add_headers)
{
	QByteArray response;
	try
	{
		response = HttpRequestHandler(HttpRequestHandler::ProxyType::NONE).post(Helper::serverApiUrl()+ "login", content.toLocal8Bit(), add_headers);
	}
	catch (Exception& e)
	{
		qDebug() << "Login problem: " + e.message();
	}
	return response;
}

QString LoginManager::user()
{
	return instance().user_;
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

QString LoginManager::userLogin()
{
	QString user_login = instance().user_login_;
	if (user_login.isEmpty()) THROW(ProgrammingException, "Cannot use LoginManager::userLogin if no user is logged in!");

	return user_login;
}

QString LoginManager::token()
{
	QString token = instance().token_;
	if (token.isEmpty()) THROW(ProgrammingException, "Cannot use LoginManager::token if no user is logged in!");

	return token;
}

QString LoginManager::password()
{
	QString password = instance().password_;
	if (password.isEmpty()) THROW(ProgrammingException, "Cannot use LoginManager::password if no user is logged in!");

	return password;
}

QString LoginManager::role()
{
	return instance().role_;
}

bool LoginManager::active()
{
	return !instance().user_.isEmpty();
}

void LoginManager::login(QString user, QString password, bool test_db)
{
	NGSD db(test_db);

	//login
	LoginManager& manager = instance();
	manager.user_id_ = db.userId(user, true);
	manager.user_ = user;
	manager.user_name_ = db.userName(manager.user_id_);
	manager.password_ = password;

	//determine role
	manager.role_ = db.getValue("SELECT user_role FROM user WHERE id='" + QString::number(manager.user_id_) + "'").toString();
	manager.user_login_ = db.userLogin(manager.user_id_);
	//update last login
	db.getQuery().exec("UPDATE user SET last_login=NOW() WHERE id='" + QString::number(manager.user_id_) + "'");

	if (!Settings::string("server_host", true).isEmpty())
	{
		HttpHeaders add_headers;
		add_headers.insert("Accept", "text/plain");
		QString content = "name="+manager.user_login_+"&password="+manager.password_;
		manager.token_ = sendAuthRequest(content, add_headers);
	}
}

void LoginManager::renewLogin()
{
	qDebug() << "Token renewal";
	if (!Settings::string("server_host", true).isEmpty())
	{
		HttpHeaders add_headers;
		add_headers.insert("Accept", "text/plain");

		LoginManager& manager = instance();
		if ((manager.user_.isEmpty()) || (manager.password_.isEmpty())) return;

		QString content = "name="+manager.user_+"&password="+manager.password_;
		manager.token_ = sendAuthRequest(content, add_headers);
	}
}

void LoginManager::logout()
{
	LoginManager& manager = instance();
	manager.user_.clear();
	manager.user_id_ = -1;
	manager.role_.clear();
	manager.token_.clear();
}

void LoginManager::checkRoleIn(QStringList roles)
{
	//check that role list is valid
	NGSD db;
	QStringList valid_roles = db.getEnum("user", "user_role");
	foreach(QString role, roles)
	{
		if (!valid_roles.contains(role)) THROW (ProgrammingException, "Invalid role '" + role + "' in LoginManager!");
	}

	//check if user has role
	LoginManager& manager = instance();
	if (!roles.contains(manager.role_))
	{
		THROW(Exception, "Access denied.\nOnly users with the roles '" + roles.join("', '") + "' have access.\nThe user '" + manager.user_ + "' has the role '" + manager.role_ + "'!");
		return;
	}
}
