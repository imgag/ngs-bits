#include "CommandServer.h"

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
    // sock.removeServer(socket_path);
    // this->socket_path = socket_path;

    this->sock = CommandListener::fromString(socket_path, this);

    // connect(&sock, SIGNAL(newConnection()), this, SLOT(newConnection()));
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
            auto line = socket->readLine(0xff);

            if (!line.isValidUtf8()) return;

            ApiCommand cmd = parseCommand(QString::fromUtf8(line));

            pending->push_back(cmd.id);

            emit commandReceived(cmd);
        }
    });
    connect(this, &CommandServer::responseReady, socket, [socket, pending](const CommandResult &res){
        if (pending->empty() || pending->front() != res.id)
            return;   // not ours, or out of order

        pending->pop_front();

        QString out = res.output.trimmed();

        if (out.isEmpty()) out = "OK";

        socket->write((out + "\n").toUtf8());
    });
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

    QDateTime now;

    return ApiCommand {
        now,
        cmd_id++,
        verb,
        arguments,
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
