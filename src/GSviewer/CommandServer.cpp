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
    QUrl url = QUrl::fromUserInput(cmd.arguments[1].sliced(1, cmd.arguments[1].length() - 2).trimmed());

    if (url.scheme().isEmpty()) url.setScheme("http");

    accessTokens.insert(url.host(), cmd.arguments[0].trimmed());

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

    if (url.scheme().startsWith("http") || url.scheme().startsWith("https")) {
        QUrlQuery query(url);

        if (accessTokens.contains(url.host()))
            query.addQueryItem("token", accessTokens.value(url.host()));


        url.setQuery(query);
    }

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
            isQuoted = !isQuoted;

            if (!isQuoted) {
                split << token.trimmed();
                token.clear();
            }
        } else
            token.push_back(c);
    }

    if (!token.trimmed().isEmpty())
        split << token.trimmed();

    return split;
}
