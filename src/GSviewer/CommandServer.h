#ifndef COMMANDSERVER_H
#define COMMANDSERVER_H

#include <QString>
#include <QDebug>
#include <QObject>
#include <QQueue>
#include <QTimer>
#include <QDateTime>
#include <QMutex>
#include <QMutexLocker>
#include <QLocalServer>
#include <QElapsedTimer>
#include <QPointer>

struct CommandResult {
    quint32 id;
    bool success;
    QString output;
};

struct GSVCommand {
    /// Timestamp of when the command was parsed (not when it was dispatched or handled)
    QDateTime received;

    /// A Serial ID representing the command's ordering
    quint32 id;

    /// The verb with which the command was invoked. (The first argument).
    QString verb;

    /// The remaining command text after the verb.
    QString arguments;
};

class CommandServer : public QObject
{
    Q_OBJECT

private:
    QLocalServer sock;
    quint32 cmd_id = 0;

    GSVCommand parseCommand(QString cmd);

public:
    explicit CommandServer(QObject *parent = nullptr);

    void bind();
    void terminate();
    void dispatchResponse(GSVCommand cmd);
    void dispatchResponse(GSVCommand cmd, QString data);
    void dispatchFailure(GSVCommand cmd, QString failure);

signals:
    void commandReceived(GSVCommand cmd);
    void responseReady(CommandResult res);

protected slots:
    void newConnection();
};

#endif // COMMANDSERVER_H
