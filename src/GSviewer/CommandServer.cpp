#include "CommandServer.h"

#include <QUrlQuery>
#include <QFile>
#include <QLocalServer>
#include <QLocalSocket>
#include <QMessageBox>
#include <QRegularExpression>
#include <deque>

using namespace Qt::StringLiterals;

CommandServer::CommandServer(QString socket_path, QObject* parent)
	: QObject{parent}
{
	sock_ = CommandListener::fromString(socket_path, this);

	connect(sock_.get(), &CommandListener::newConnection, this, &CommandServer::newConnection);
}

void CommandServer::bind()
{
	sock_->bind();
}

void CommandServer::terminate()
{
	sock_->terminate();
}

void CommandServer::newConnection(QIODevice* socket)
{
	auto pending = std::make_shared<std::deque<quint32>>();

	connect(socket, &QIODevice::readyRead, socket, [this, socket, pending]
	{
		while (socket->canReadLine())
		{
			auto line = socket->readLine();

			if (!line.isValidUtf8())
			{
				this->dispatchFailure(ApiCommand{QDateTime::currentDateTime(), cmd_id_++, "", splitQuoted("")}, "Invalid UTF-8 sequence received.");
				continue;
			}

			ApiCommand cmd = parseCommand(QString::fromUtf8(line));

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
	QUrl url = QUrl::fromUserInput(cmd.arguments[1].sliced(1, cmd.arguments[1].length() - 2).trimmed());

	if (url.scheme().isEmpty()) url.setScheme("http");

	access_tokens_.insert(url.host(), cmd.arguments[0].trimmed());

	dispatchResponse(cmd);
}

ApiCommand CommandServer::parseCommand(QString cmd)
{
	QRegularExpression split("\\s+");

	auto verb_start = cmd.indexOf(split);

	if (verb_start <= -1)
	{
		verb_start = cmd.length();
	}

	auto verb = cmd.sliced(0, verb_start).trimmed();
	auto arguments = cmd.sliced(verb_start).trimmed();

	if (verb == "debug")
	{
		QElapsedTimer duration;
		duration.start();

		qDebug() << "Received DEBUG command";

		auto cmd = parseCommand(arguments);

		return cmd;
	}

	return ApiCommand{
		QDateTime::currentDateTime(),
		cmd_id_++,
		verb,
		splitQuoted(arguments),
	};
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

QStringList CommandServer::splitQuoted(QString str)
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
			split << token.trimmed();
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
