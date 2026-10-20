#ifndef COMMANDLISTENER_H
#define COMMANDLISTENER_H

#include <QLocalServer>
#include <QObject>
#include <QTcpServer>

class CommandListener : public QObject
{
    Q_OBJECT

protected:
    explicit CommandListener(QString addr, QObject *parent = nullptr);

public:
    static std::unique_ptr<CommandListener> fromString(const QString &address, QObject* parent = nullptr);

    virtual void bind() = 0;
    virtual void terminate() = 0;

protected:
    QString address;

signals:
    void newConnection(QIODevice* socket);
    void closedConnection(QIODevice* socket);
};

class TcpListener : public CommandListener {
private:
    QTcpServer sock;
    quint16 port;

public:
    TcpListener(const quint16 port, QObject *parent = nullptr);

    void bind() override;
    void terminate() override;
};

class LocalListener : public CommandListener {
private:
    QLocalServer sock;

public:
    LocalListener(const QString &path, QObject *parent);

    void bind() override;
    void terminate() override;
};

#endif // COMMANDLISTENER_H
