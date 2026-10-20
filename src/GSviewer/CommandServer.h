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
#include <CommandListener.h>

struct CommandResult {
    quint32 id;
    bool success;
    QString output;
};

struct ApiCommand {
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
    std::unique_ptr<CommandListener> sock;
    quint32 cmd_id = 0;

    ApiCommand parseCommand(QString cmd);

public:
    explicit CommandServer(QString socket_path = "", QObject *parent = nullptr);

    void bind();
    void terminate();

    void dispatchResponse(ApiCommand cmd);
    void dispatchResponse(ApiCommand cmd, QString data);
    void dispatchFailure(ApiCommand cmd, QString failure);

    static QStringList split_quoted(QString);

signals:
    void commandReceived(ApiCommand cmd);
    void responseReady(CommandResult res);

protected slots:
    void newConnection(QIODevice* sock);
};

#endif // COMMANDSERVER_H
