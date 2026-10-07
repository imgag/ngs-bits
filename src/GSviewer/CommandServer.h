#ifndef COMMANDSERVER_H
#define COMMANDSERVER_H

#include <QObject>
#include <QLocalServer>

/// Represents an action acceptable in command form. Aimed at interoperability with [IGV commands](https://github.com/igvteam/igv/wiki/Batch-commands/32fbbbf3c7b7c79a5e531f03346c4624716422b0)
enum class CmdVerb {
    New,
    Echo,
    Load,
    Goto,
    Genome,
};


struct GSVCommand {
    /// The complete, original, unmodified command string. Useful for debugging purposes.
    QString text;

    /// The verb with which the command was invoked. (The first argument).
    CmdVerb verb;

    /// The remaining command text after the verb.
    QString arguments;
};


class CommandServer : public QObject
{
    Q_OBJECT

private:
    QLocalServer sock;

    void parseCommand(QString cmd);

public:
    explicit CommandServer(QObject *parent = nullptr);

    void bind();
    void terminate();

signals:
    void commandReceived(GSVCommand cmd);

protected slots:
    void newConnection();
};

#endif // COMMANDSERVER_H
