#ifndef IGVINITWORKER_H
#define IGVINITWORKER_H

#include <QRunnable>
#include <QFile>
#include <QProcess>
#include <QDateTime>
#include <QAbstractSocket>
#include <QTcpSocket>

#include "Exceptions.h"
#include "Log.h"

class IGVInitWorker
    : public QRunnable
{
public:
    IGVInitWorker(const QString& igv_host, const int& igv_port, const QString& igv_app);
    void run();

protected:
    QString igv_host_;
    int igv_port_;
    QString igv_app_;
};

#endif // IGVINITWORKER_H
