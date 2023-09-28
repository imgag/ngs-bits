#ifndef IGVCOMMANDEXECUTOR_H
#define IGVCOMMANDEXECUTOR_H

#include <QThreadPool>
#include <QAbstractSocket>
#include <QFile>
#include <QProcess>
#include <QEventLoop>
#include <QMessageBox>

#include "Exceptions.h"
#include "Settings.h"
#include "IGVCommandWorker.h"
#include "IGVInitWorker.h"

class IGVCommandExecutor : public QObject
{
    Q_OBJECT

public:
    IGVCommandExecutor(const QString& igv_app, const QString& igv_host, const int& igv_port);
    void initIGV();
    void execute(const QStringList& commands);
    bool isIgvRunning();
    bool hasRunningCommands();
    QStringList getErrorMessages();
    QStringList getHistory();
    void clearHistory();

signals:
    void historyUpdated(QStringList updated_history);

public slots:
    void handleExecptions(QString message);
    void addToHistory(QString status);

private:
    QThreadPool execution_pool_;
    QString igv_host_;
    int igv_port_;
    QString igv_app_;
    bool is_igv_running_;
    QStringList responses_;
    QStringList error_messages_;
    QStringList command_history_;
};

#endif // IGVCOMMANDEXECUTOR_H
