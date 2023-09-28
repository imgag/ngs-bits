#include "IGVCommandExecutor.h"

IGVCommandExecutor::IGVCommandExecutor(const QString& igv_app, const QString& igv_host, const int& igv_port)
    : igv_app_(igv_app)
    , igv_host_(igv_host)
    , igv_port_(igv_port)
    , is_igv_running_(false)
    , responses_({})
    , error_messages_()
{
    execution_pool_.setMaxThreadCount(1);
}

void IGVCommandExecutor::initIGV()
{
    IGVInitWorker* init_worker = new IGVInitWorker(igv_host_, igv_port_, igv_app_);
    execution_pool_.start(init_worker);
}

void IGVCommandExecutor::execute(const QStringList& commands)
{
    int execution_dealay_ms = 5000;

    if (commands.count() == 1) execution_dealay_ms = 0;
    IGVCommandWorker* command_worker = new IGVCommandWorker(igv_host_, igv_port_, commands, responses_, execution_dealay_ms);
    connect(command_worker, SIGNAL(commandFailed(QString)), this, SLOT(handleExecptions(QString)));
    connect(command_worker, SIGNAL(commandReported(QString)), this, SLOT(addToHistory(QString)));
    execution_pool_.start(command_worker);
}

bool IGVCommandExecutor::isIgvRunning()
{
    Log::info("IGV availability check");
    QStringList commands = {"echo running"};

    IGVCommandWorker* command_worker = new IGVCommandWorker(igv_host_, igv_port_, commands, responses_, 0);
    execution_pool_.start(command_worker);
    execution_pool_.waitForDone();

    if (responses_.count()>0)
    {
        if (responses_[0].toLower() == "running") return true;
    }

    return false;
}

bool IGVCommandExecutor::hasRunningCommands()
{
    return (execution_pool_.activeThreadCount()>0);
}

QStringList IGVCommandExecutor::getErrorMessages()
{
    return error_messages_;
}

QStringList IGVCommandExecutor::getHistory()
{
    if (execution_pool_.activeThreadCount()>0) return QStringList{};
    return command_history_;
}

void IGVCommandExecutor::clearHistory()
{
    if (execution_pool_.activeThreadCount()>0) return;
    command_history_.clear();
}

void IGVCommandExecutor::addToHistory(QString status)
{
    command_history_.append(status);
    emit historyUpdated(command_history_);
}

void IGVCommandExecutor::handleExecptions(QString message)
{
    Log::error(message);
    error_messages_.append(message);
}
