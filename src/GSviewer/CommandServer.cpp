#include "CommandServer.h"

#include <QFile>
#include <QLocalServer>
#include <QLocalSocket>
#include <QMessageBox>
#include <QRegularExpression>
#include <deque>

using namespace Qt::StringLiterals;

const QString tmp_path = QString::fromUtf8("\\\\.\\gsvar\\cmd.sock");
static const quint32 DISPATCH_INTERVAL_MS = 50;

CommandServer::CommandServer(QObject *parent)
    : QObject{parent}
{
    sock.removeServer(tmp_path);

    connect(&sock, SIGNAL(newConnection()), this, SLOT(newConnection()));
    connect(&dispatchInterval, SIGNAL(timeout()), this, SLOT(dispatchEnqueued()));

    dispatchInterval.setInterval(DISPATCH_INTERVAL_MS);
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

        qDebug() << "Starting Command Interval timer";
        dispatchInterval.start();

        connect(client, &QLocalSocket::disconnected, client, &QObject::deleteLater);
        connect(client, &QLocalSocket::disconnected, this, [this]() {
            // Stop the timer if there are no more incomming connections to save on CPU cycles.
            if (!sock.hasPendingConnections()) {
                qDebug() << "Pausing Command Interval timer";
                dispatchInterval.stop();
            }
        });
        connect(client, &QLocalSocket::readyRead, client, [this, client, pending]() {
            while (client->canReadLine()) {
                auto line = client->readLine(0xff);

                if (!line.isValidUtf8()) return;

                auto cmd = parseCommand(QString::fromUtf8(line));

                qDebug() << "Received a new command: " << cmd.verb;

                QMutexLocker locker(&cmdQueueLock);
                pending->push_back(cmd.id);
                cmdQueueWrite.enqueue(cmd);
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

    auto arguments = cmd.sliced(verbStart).trimmed();

    static const QHash<QString, CmdVerb> verbs = {
        { u"new"_s, CmdVerb::New },
        { u"echo"_s, CmdVerb::Echo },
        { u"load"_s, CmdVerb::Load },
        { u"goto"_s, CmdVerb::Goto },
        { u"genome"_s, CmdVerb::Genome },
        { u"answer"_s, CmdVerb::Answer },
        { u"debug"_s, CmdVerb::Debug },
    };

    auto verb = verbs.value(cmd.sliced(0, verbStart).trimmed());

    if (verb == CmdVerb::Debug) {
        QElapsedTimer duration;
        duration.start();

        qDebug() << "Received DEBUG command";

        auto cmd = parseCommand(arguments);
        cmd.debug = true;

        return cmd;
    }

    return GSVCommand {
        cmd_id++,
        cmd,
        verb,
        arguments,
        false
    };
}

void CommandServer::dispatchEnqueued() {
    // stop the timer while we handle events, in case we overrun the time slot in which to handle the commands.
    // In this case, the mutex will handle draining the queue, but not dispatching events.

    if (cmdQueueWrite.isEmpty())
        return;

    qDebug() << "Processing all " << cmdQueueWrite.length() << " enqueued commands";
    dispatchInterval.stop();

    {
        QMutexLocker lock(&cmdQueueLock);

        // Drain the queue so that we can continue receiving events while we process.
        cmdQueueRead.swap(cmdQueueWrite);
    }

    for (const auto &cmd : cmdQueueRead)
        emit commandReceived(cmd);

    cmdQueueRead.clear();
    dispatchInterval.start();
    qDebug() << "Resuming Command Interval timer";
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
