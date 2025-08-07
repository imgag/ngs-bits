#ifndef CLIENTHELPER_H
#define CLIENTHELPER_H

#include "cppNGS_global.h"
#include "Settings.h"
#include "Helper.h"
#include "HttpRequestHandler.h"
#include "Log.h"
#include <QString>
#include <QDateTime>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>


//Contains information about the GSvarServer (used in client-server mode)
struct CPPNGSSHARED_EXPORT ServerInfo
{
	QString version; // server version
	QString api_version; // server API version
	QDateTime server_start_time; // date and time when the server was started
    QString server_url; // server host / domain name

	bool isEmpty()
	{
		return version.isEmpty() && api_version.isEmpty() && server_start_time.isNull();
	}
};

// Information about the current version of the desktop clinet application (e.g. needed to inform the user
// about updates)
struct CPPNGSSHARED_EXPORT ClientInfo
{
	QString version;
	QString message;
	QDateTime date;

	ClientInfo()
		: version()
		, message()
		, date()
	{
	}

	ClientInfo(const QString& version_, const QString& message_, const QDateTime date_ = QDateTime::currentDateTime())
		: version(version_)
		, message(message_)
		, date(date_)
	{
	}

	QList<int> getVersionParts()
	{
		QList<int> output;
		QStringList version_present_parts_start = version.split("_");
		if (version_present_parts_start.size()>1)
		{
			output.append(version_present_parts_start[0].toInt());
			QStringList version_present_parts_end = version_present_parts_start[1].split("-");
			if (version_present_parts_end.size()>1)
			{
				output.append(version_present_parts_end[0].toInt());
				output.append(version_present_parts_end[1].toInt());
			}
		}
		if (output.size()<3) output = QList<int> {0, 0, 0};
		return output;
	}

	bool isOlderThan(ClientInfo info_provided)
	{
		QList<int> present = getVersionParts();
		QList<int> provided = info_provided.getVersionParts();

		bool is_older = false;

		if (present[0]<provided[0]) is_older = true;
		if (present[0]>provided[0]) return false;
		if (!is_older && (present[1]<provided[1])) is_older = true;
		if (present[1]>provided[1]) return false;
		if (!is_older && (present[2]<provided[2])) is_older = true;
		return is_older;
	}

	bool isOlderThan(QString version)
	{
		return isOlderThan(ClientInfo(version,""));
	}

	QJsonObject toJsonObject()
	{
		QJsonObject out;
		out.insert("version", version);
		out.insert("message", message);
		out.insert("date", date.toSecsSinceEpoch());
		return out;
	}

	bool isEmpty()
	{
		return version.isEmpty() && message.isEmpty() && date.isNull();
	}
};

// Stores a notification displayed to the user of the client application (e.g. downtimes, maintenance, updates)
struct CPPNGSSHARED_EXPORT UserNotification
{
	QString id;
	QString message;

	UserNotification()
		: id()
		, message()
	{
	}

	UserNotification(QString id_, QString message_)
		: id(id_)
		, message(message_)
	{
	}

	bool isEmpty()
	{
		return id.isEmpty() && message.isEmpty();
	}
};

// Contains methods to enable simpler interaction between the clinet and the server:
// information about API, server, current client, JSON serialization, etc.
class CPPNGSSHARED_EXPORT ClientHelper
{
public:
	///Returns if the application is running in client-server mode (mainly used for GSvar).
	static bool isClientServerMode();
	///Checks if the application is running on the server or on a client machine
	static bool isRunningOnServer();
	///Checks if a given local file or URL is a BAM file
	static bool isBamFile(QString filename);
	///Removes a secure token from the URL that is given to IGV
	static QString stripSecureToken(QString url);

	///Requests information about GSvarServer
    static ServerInfo getServerInfo(int& status_code);
	///Requests the client version number from the server to inform about updates (if available)
	static ClientInfo getClientInfo();
	///Requests the user notification from the server
	static UserNotification getUserNotification();
	///Returns the server API version. Used to check that the server and the client have the same version.
	static QString serverApiVersion();
	///Returns the URL used for sending requests to the GSvar server (use only when in client-server mode)
	static QString serverApiUrl();

private:
	///Constructor declared away
	ClientHelper() = delete;
};

#endif // CLIENTHELPER_H
