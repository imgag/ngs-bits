#include "CommandServer.h"
#include "Exceptions.h"
#include <QRegularExpression>
#include <QUrlQuery>
#include <deque>
#include <QTcpSocket>

CommandServer::CommandServer(QObject* parent)
	: QObject{parent}
	, server_(this)
{
	connect(&server_, &QTcpServer::newConnection, this, [this]
			{
				while (auto* s = server_.nextPendingConnection())
				{
					connect(s, &QTcpSocket::disconnected, this, [s]
							{
								s->deleteLater();
							});
					emit newConnection(s);
				}
			});
}

void CommandServer::open(int port)
{
	bool ok = server_.listen(QHostAddress::LocalHost, port);
	if (!ok) THROW(ArgumentException, "Could not open port " + QString::number(port));
}

void CommandServer::close()
{
	server_.close();
}

int CommandServer::port() const
{
	if (!server_.isListening()) return -1;
	return server_.serverPort();
}

void CommandServer::newConnection(QIODevice* socket)
{
	auto pending = std::make_shared<std::deque<quint32>>();

	connect(socket, &QIODevice::readyRead, socket, [this, socket, pending]
	{
		while (socket->canReadLine())
		{
			QByteArray line = socket->readLine();

			if (!line.isValidUtf8())
			{
				this->dispatchFailure(ApiCommand{QDateTime::currentDateTime(), cmd_id_++, "", QStringList()}, "Invalid UTF-8 sequence received.");
				pending->push_back(cmd_id_);
				continue;
			}

			ApiCommand cmd = parseCommand(line);
			pending->push_back(cmd.id);
			if (cmd.verb == "SetAccessToken")
			{
				handleAccessToken(cmd);
			}
			else
			{
				emit commandReceived(cmd);
			}
		}
	});
	connect(this, &CommandServer::responseReady, socket, [socket, pending](const CommandResult& res)
	{
		if (pending->empty() || pending->front() != res.id)
		{
			return; // not ours, or out of order
		}

		pending->pop_front();

		QString out = res.output.trimmed();

		if (out.isEmpty()) out = "OK";

		socket->write((out + "\n").toUtf8());
	});
}

void CommandServer::handleAccessToken(ApiCommand cmd)
{
	if (cmd.arguments.length() != 2) return dispatchFailure(cmd, "Expected exactly 2 arguments. Got " + QString::number(cmd.arguments.length()));
	if (cmd.arguments[1].length() < 2) return dispatchFailure(cmd, "A URL of length <2 was received. This is very likely invalid.");

	QUrl url = QUrl::fromUserInput(cmd.arguments[1].sliced(1, cmd.arguments[1].length() - 2).trimmed());

	if (url.scheme().isEmpty()) url.setScheme("https");

	access_tokens_.insert(url.host(), cmd.arguments[0].trimmed());

	dispatchResponse(cmd);
}

ApiCommand CommandServer::parseCommand(QString cmd)
{
	//split command and args
	cmd = cmd.simplified();
	int i_sep = cmd.indexOf(' ');
	if (i_sep <= -1) i_sep = cmd.length();
	QString verb = cmd.sliced(0, i_sep).trimmed();
	QString arguments = cmd.sliced(i_sep).trimmed();

	return ApiCommand{ QDateTime::currentDateTime(), cmd_id_++, verb, splitQuoted(arguments) };
}

void CommandServer::dispatchResponse(ApiCommand cmd)
{
	emit responseReady(CommandResult{
		cmd.id,
		true,
		"OK",
	});
}

void CommandServer::dispatchResponse(ApiCommand cmd, QString data)
{
	emit responseReady(CommandResult{
		cmd.id,
		true,
		data,
	});
}

void CommandServer::dispatchFailure(ApiCommand cmd, QString failure)
{
	emit responseReady(CommandResult{
		cmd.id,
		false,
		failure,
	});
}

QUrl CommandServer::parseUrl(QString str)
{
	QUrl url = QUrl::fromUserInput(str);

	if (url.scheme().startsWith("http") || url.scheme().startsWith("https"))
	{
		QUrlQuery query(url);

		if (access_tokens_.contains(url.host()))
		{
			query.addQueryItem("token", access_tokens_.value(url.host()));
		}


		url.setQuery(query);
	}

	return url;
}

QStringList CommandServer::splitQuoted(QString str) //TODO Marc easier with Qt?
{
	QStringList split;

	bool is_quoted = false;
	QString token;
	for (qsizetype i = 0; i < str.size(); i++)
	{
		const QChar c = str[i];

		// Handle escaped string characters
		if (c == '\\')
		{
			// This has the nice unintended consequence that a string terminated by an escaped quote doesn't cause errors.
			// Instead, the escaped quote is simply emitted, the string then terminates normally due to the end of the input and all is well.
			if (i >= str.size() - 1)
			{
				token.push_back("\\");
				continue;
			}

			const QChar next = str.at(++i);

			if (next == '"')
			{
				token.push_back('"');
			}
			else if (next == '\\')
			{
				token.push_back('\\');
			}
			else if (next == 'n')
			{
				token.push_back('\n');
			}
			else
			{
				token.push_back('\\');
				token.push_back(next);
			}
		}
		else if (c.isSpace() && !is_quoted)
		{
			if (!token.trimmed().isEmpty()) split << token.trimmed();

			token.clear();
		}
		else if (c == '"')
		{
			is_quoted = !is_quoted;

			if (!is_quoted)
			{
				split << token.trimmed();
				token.clear();
			}
		}
		else
		{
			token.push_back(c);
		}
	}

	if (!token.trimmed().isEmpty())
	{
		split << token.trimmed();
	}

	return split;
}
