#include "CommandServer.h"

#include <QUrlQuery>
#include <QFile>
#include <QLocalServer>
#include <QLocalSocket>
#include <QMessageBox>
#include <QRegularExpression>
#include <deque>

using namespace Qt::StringLiterals;

CommandServer::CommandServer(QString socket_path, QObject *parent)
    : QObject{parent}
{
    this->sock = CommandListener::fromString(socket_path, this);

    connect(sock.get(), &CommandListener::newConnection, this, &CommandServer::newConnection);
}

void CommandServer::bind() {
    sock->bind();
}

void CommandServer::terminate() {
    sock->terminate();
}

void CommandServer::newConnection(QIODevice* socket) {
    auto pending = std::make_shared<std::deque<quint32>>();

    connect(socket, &QIODevice::readyRead, socket, [this, socket, pending] {
        while (socket->canReadLine()) {
            auto line = socket->readLine();

            if (!line.isValidUtf8()) {
                this->dispatchFailure(ApiCommand { QDateTime::currentDateTime(), cmd_id++, "", split_quoted("") }, "Invalid UTF-8 sequence received.");
                continue;
            }

            ApiCommand cmd = parseCommand(QString::fromUtf8(line));

            pending->push_back(cmd.id);

            if (cmd.verb == "SetAccessToken") {
                handleAccessToken(cmd);
            } else {
                emit commandReceived(cmd);
            }
        }
    });
    connect(this, &CommandServer::responseReady, socket, [socket, pending](const CommandResult &res){
        if (pending->empty() || pending->front() != res.id)
            return; // not ours, or out of order

        pending->pop_front();

        QString out = res.output.trimmed();

        if (out.isEmpty()) out = "OK";

        socket->write((out + "\n").toUtf8());
    });
}

void CommandServer::handleAccessToken(ApiCommand cmd) {
    for (auto arg : cmd.arguments) {
        auto split = arg.lastIndexOf(" *");

        auto token = arg.sliced(0, split).trimmed();
        auto urlFragment = arg.sliced(split).trimmed();

        if (urlFragment.startsWith('*'))
            urlFragment = urlFragment.sliced(1);

        if (urlFragment.endsWith('*'))
            urlFragment = urlFragment.sliced(0, urlFragment.length() - 1);

        QUrl url = QUrl::fromUserInput(urlFragment);

        if (url.scheme().isEmpty()) {
            url.setScheme("http");
        }

        accessTokens.insert(url.host(), token);
    }

    dispatchResponse(cmd);
}

ApiCommand CommandServer::parseCommand(QString cmd) {
    QRegularExpression split("\\s+");

    auto verbStart = cmd.indexOf(split);

    if (verbStart <= -1)
        verbStart = cmd.length();

    auto verb = cmd.sliced(0, verbStart).trimmed();
    auto arguments = cmd.sliced(verbStart).trimmed();

    if (verb == "debug") {
        QElapsedTimer duration;
        duration.start();

        qDebug() << "Received DEBUG command";

        auto cmd = parseCommand(arguments);

        return cmd;
    }

    return ApiCommand {
        QDateTime::currentDateTime(),
        cmd_id++,
        verb,
        split_quoted(arguments),
    };
}

void CommandServer::dispatchResponse(ApiCommand cmd) {
    emit responseReady(CommandResult {
        cmd.id,
        true,
        "OK",
    });
}

void CommandServer::dispatchResponse(ApiCommand cmd, QString data) {
    emit responseReady(CommandResult {
        cmd.id,
        true,
        data,
    });
}

void CommandServer::dispatchFailure(ApiCommand cmd, QString failure) {
    emit responseReady(CommandResult {
        cmd.id,
        false,
        failure,
    });
}

QUrl CommandServer::parseUrl(QString str) {
    QUrl url = QUrl::fromUserInput(str);

    QUrlQuery query(url);

    if (accessTokens.contains(url.host()))
        query.addQueryItem("token", accessTokens.value(url.host()));

    url.setQuery(query);

    qDebug() << url;

    return url;
}

QStringList CommandServer::split_quoted(QString str) {
    QStringList split;

    bool isQuoted = false;
    QString token;
    for (qsizetype i = 0; i < str.size(); i++) {
        const QChar c = str[i];

        // Handle escaped string characters
        if (c == '\\') {
            const QChar next = str.at(++i);

            if (next == '"')
                token.push_back('"');

            else if (next == '\\')
                token.push_back('\\');

            else if (next == 'n')
                token.push_back('\n');

            else {
                token.push_back('\\');
                token.push_back(next);
            }
        } else if (c.isSpace() && !isQuoted) {
            split << token.trimmed();
            token.clear();
        } else if (c == '"') {
            if (isQuoted) {
                split << token.trimmed();
                token.clear();
                isQuoted = false;
            } else {
                isQuoted = true;
            }
        } else
            token.push_back(c);
    }

    if (!token.trimmed().isEmpty())
        split << token.trimmed();

    qDebug() << split;

    return split;
}
