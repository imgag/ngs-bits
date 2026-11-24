#ifndef COMMANDSERVER_H
#define COMMANDSERVER_H

#include <QString>
#include <QObject>
#include <QDateTime>
#include <QUrl>
#include <QTcpServer>

struct CommandResult
{
	quint32 id;
	bool success;
	QString output;
};

struct ApiCommand
{
	QDateTime received; // Timestamp of when the command was parsed (not when it was dispatched or handled)
	quint32 id; // A Serial ID representing the command's ordering
	QString verb; // The verb with which the command was invoked. (The first argument).
	QStringList arguments; // The remaining command text after the verb.
};

class CommandServer
	: public QObject
{
	Q_OBJECT

public:
	CommandServer(QObject* parent = nullptr);

	void open(int port);
	void close();
	//Returns the port currently listened on. If no connection is open, returns -1;
	int port() const;

	void dispatchResponse(ApiCommand cmd);
	void dispatchResponse(ApiCommand cmd, QString data);
	void dispatchFailure(ApiCommand cmd, QString failure);

	static QStringList splitQuoted(QString);

	QUrl parseUrl(QString str);

signals:
	void commandReceived(ApiCommand cmd);
	void responseReady(CommandResult res);

protected slots:
	void newConnection(QIODevice* socket);

private:
	QTcpServer server_;
	quint32 cmd_id_ = 0;
	QHash<QString, QString> access_tokens_;

	ApiCommand parseCommand(QString cmd);
	void handleAccessToken(ApiCommand cmd);
};

#endif // COMMANDSERVER_H
