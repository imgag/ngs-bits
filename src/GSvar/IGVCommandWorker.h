#ifndef IGVCOMMANDWORKER_H
#define IGVCOMMANDWORKER_H

#include <QRunnable>
#include <QDateTime>
#include <QTcpSocket>
#include <QTimer>
#include <QEventLoop>

#include "Exceptions.h"
#include "Log.h"

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

protected:   
    QString igv_host_;
    int igv_port_;
    QStringList commands_;
    QStringList& responses_;
    int delay_ms_;
};

#endif // IGVCOMMANDWORKER_H
