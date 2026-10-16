#ifndef COMMANDSERVER_H
#define COMMANDSERVER_H

#include <QString>
#include <QDebug>
#include <QObject>
#include <QQueue>
#include <QTimer>
#include <QMutex>
#include <QMutexLocker>
#include <QLocalServer>
#include <QElapsedTimer>
#include <QPointer>

/// Represents an action acceptable in command form. Aimed at interoperability with [IGV commands](https://github.com/igvteam/igv/wiki/Batch-commands/32fbbbf3c7b7c79a5e531f03346c4624716422b0)
enum class CmdVerb {
    New,
    Echo,
    Load,
    Goto,
    Genome,
    Answer,
    Debug,
};

inline QString toString(CmdVerb v)
{
    switch (v) {
        case CmdVerb::New:    return QStringLiteral("new");
        case CmdVerb::Echo:   return QStringLiteral("echo");
        case CmdVerb::Load:   return QStringLiteral("load");
        case CmdVerb::Goto:   return QStringLiteral("goto");
        case CmdVerb::Genome: return QStringLiteral("genome");
        case CmdVerb::Answer: return QStringLiteral("answer");
        case CmdVerb::Debug:  return QStringLiteral("debug");
    }
    Q_UNREACHABLE();
    return {};
}

// Makes qDebug() << verb work
inline QDebug operator<<(QDebug dbg, CmdVerb v)
{
    QDebugStateSaver saver(dbg);
    dbg.noquote() << toString(v);
    return dbg;
}

struct CommandResult {
    quint32 id;
    bool success;
    QString output;
};


struct GSVCommand {
    /// A Serial ID representing the command's ordering
    quint32 id;

    /// The complete, original, unmodified command string. Useful for debugging purposes.
    QString text;

    /// The verb with which the command was invoked. (The first argument).
    CmdVerb verb;

    /// The remaining command text after the verb.
    QString arguments;

    /// Whether the command should be run in debug mode
    bool debug;
};


class CommandServer : public QObject
{
    Q_OBJECT

private:
    QLocalServer sock;
    quint32 cmd_id = 0;

    GSVCommand parseCommand(QString cmd);

    QQueue<GSVCommand> cmdQueueWrite;
    QQueue<GSVCommand> cmdQueueRead;
    QMutex cmdQueueLock;
    QTimer dispatchInterval;

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
    void dispatchEnqueued();
};

#endif // COMMANDSERVER_H
