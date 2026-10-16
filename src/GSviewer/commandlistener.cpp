#include "CommandListener.h"

#include <QLocalSocket>
#include <QTcpSocket>

std::unique_ptr<CommandListener> CommandListener::fromString(const QString &addr, QObject* parent) {
    bool ok = false;
    const quint16 port = addr.toUShort(&ok);

    if (ok)
        return std::make_unique<TcpListener>(port, parent);
    else
        return std::make_unique<LocalListener>(addr, parent);
}

CommandListener::CommandListener(const QString addr, QObject* parent) : QObject(parent), address(addr) {
    connect(this, &CommandListener::disconnect, this, [](QIODevice* sock) {
        sock->deleteLater();
    });
}

TcpListener::TcpListener(const quint16 port, QObject* parent) : CommandListener(QString::number(port), parent), port(port) {
    connect(&sock, &QTcpServer::newConnection, this, [this] {
        while (auto *s = sock.nextPendingConnection()) {
            connect(s, &QTcpSocket::disconnected, this, [this, s] {
                emit disconnect(s);
            });
            emit newConnection(s);
        }
    });
}

void TcpListener::bind() {
    sock.listen(QHostAddress::LocalHost, port);
}

void TcpListener::terminate() {
    sock.close();
}

LocalListener::LocalListener(const QString &path, QObject* parent) : CommandListener(path, parent) {
    connect(&sock, &QLocalServer::newConnection, this, [this] {
        while (auto *s = sock.nextPendingConnection()) {
            connect(s, &QLocalSocket::disconnected, this, [this, s] {
                emit disconnect(s);
            });
            emit newConnection(s);
        }
    });
}

void LocalListener::bind() {
    sock.listen(address);
}

void LocalListener::terminate() {
    sock.close();
}
