#ifndef IGVCOMMANDWORKER_H
#define IGVCOMMANDWORKER_H

#include <QRunnable>
#include <QDateTime>
#include <QTcpSocket>
#include <QTimer>
#include <QEventLoop>

#include "Exceptions.h"
#include "Log.h"

//A worker class for running IGV commands. Commands are executed one after another and do not run
//in parallel. Status and errors are reported by emited signals and inside "responses" variable
class IGVCommandWorker
    : public QObject
    , public QRunnable
{
    Q_OBJECT

public:
    IGVCommandWorker(const QString& igv_host, const int& igv_port, const QStringList& commands, QStringList &responses, int delay_ms);
    void run();

signals:
    void commandFailed(QString message);
    void commandReported(QString status);
    void commandQueueStarted();
    void commandQueueFinished();

protected:   
    QString igv_host_;
    int igv_port_;
    QStringList commands_;
    QStringList& responses_;
    int delay_ms_;
};

#endif // IGVCOMMANDWORKER_H
