#ifndef QUEUINGENGINEOUTPUT_H
#define QUEUINGENGINEOUTPUT_H

#include <QString>
#include <QByteArrayList>

struct QueuingEngineOutput
{
    QString command; // command for the queuing engine
    QStringList args; // arguments for the command
    QByteArrayList result; // command output
    int exit_code; // standard exit code (0 - success)
};

#endif // QUEUINGENGINEOUTPUT_H
