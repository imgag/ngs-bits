#include "CommandServer.h"

#include <QFile>
#include <QLocalServer>
#include <QLocalSocket>
#include <QMessageBox>
#include <QRegularExpression>
#include <deque>

using namespace Qt::StringLiterals;

const QString tmp_path = QString::fromUtf8("\\\\.\\gsvar\\cmd.sock");

CommandServer::CommandServer(QObject *parent)
    : QObject{parent}
{
    sock.removeServer(tmp_path);

    connect(&sock, SIGNAL(newConnection()), this, SLOT(newConnection()));
}

void CommandServer::bind() {
    sock.listen(tmp_path);
}

void CommandServer::terminate() {
    sock.close();
}

void CommandServer::newConnection() {
    while (QLocalSocket* client = sock.nextPendingConnection()) {
        auto pending = std::make_shared<std::deque<quint32>>();

        connect(client, &QLocalSocket::disconnected, client, &QObject::deleteLater);
        connect(client, &QLocalSocket::readyRead, client, [this, client, pending]() {
            while (client->canReadLine()) {
                auto line = client->readLine(0xff);

                if (!line.isValidUtf8()) return;

                GSVCommand cmd = parseCommand(QString::fromUtf8(line));

                qDebug() << "Received a new command: " << line.trimmed();

                pending->push_back(cmd.id);

                emit commandReceived(cmd);
            }
        });

        connect(this, &CommandServer::responseReady, client, [client, pending](const CommandResult &res) {
            if (pending->empty() || pending->front() != res.id)
                return;   // not ours, or out of order

            pending->pop_front();

            QString out = res.output.trimmed();

            if (out.isEmpty()) out = "OK";

            client->write((out + "\n").toUtf8());
            client->flush();
        });
    }
}

GSVCommand CommandServer::parseCommand(QString cmd) {
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

    return GSVCommand {
        now,
        cmd_id++,
        verb,
        arguments,
    };
}

void CommandServer::dispatchResponse(GSVCommand cmd) {
    emit responseReady(CommandResult {
        cmd.id,
        true,
        "OK",
    });
}

void CommandServer::dispatchResponse(GSVCommand cmd, QString data) {
    emit responseReady(CommandResult {
        cmd.id,
        true,
        data,
    });
}

void CommandServer::dispatchFailure(GSVCommand cmd, QString failure) {
    emit responseReady(CommandResult {
        cmd.id,
        false,
        failure,
    });
}
