#include "CommandServer.h"

#include <QFile>
#include <QLocalServer>
#include <QLocalSocket>
#include <QMessageBox>
#include <QRegularExpression>

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
        connect(client, &QLocalSocket::disconnected, client, &QObject::deleteLater);
        connect(client, &QLocalSocket::readyRead, this, [this, client]() {
            while (client->canReadLine()) {
                auto line = client->readLine(0xff);

                if (line.isValidUtf8())
                    parseCommand(QString::fromUtf8(line));
            }
        });
    }
}

void CommandServer::parseCommand(QString cmd) {
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
    };

    auto verb = verbs.value(cmd.sliced(0, verbStart).trimmed());

    emit commandReceived(GSVCommand {
        cmd, verb, arguments
    });
}
