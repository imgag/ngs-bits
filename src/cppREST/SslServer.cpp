#include "SslServer.h"

SslServer::SslServer(QObject *parent)
    : QTcpServer(parent)
    , thread_pool_()
    , email_already_sent_(false)
    , thread_pool_check_count_(0)
{
	current_ssl_configuration_ = QSslConfiguration::defaultConfiguration();
    int thread_timeout = Settings::integer("thread_timeout")*1000;
    if (thread_timeout == 0)
    {
        Log::error("Thread timeout is not set or equals to zero");
        exit(1);
    }
    thread_pool_.setExpiryTimeout(thread_timeout);
    int thread_count = Settings::integer("thread_count");
    if (thread_timeout == 0)
    {
        Log::error("Max number of threads is not set or equals to zero");
        exit(1);
    }
    thread_pool_.setMaxThreadCount(thread_count);

    worker_params_.socket_read_timeout = Settings::integer("socket_read_timeout")*1000;
    if (worker_params_.socket_read_timeout == 0)
    {
        Log::error("Socket reading timeout is not set or equals to zero");
        exit(1);
    }
    worker_params_.socket_write_timeout = Settings::integer("socket_write_timeout")*1000;
    if (worker_params_.socket_write_timeout == 0)
    {
        Log::error("Socket writing timeout is not set or equals to zero");
        exit(1);
    }
    worker_params_.socket_encryption_timeout = Settings::integer("socket_encryption_timeout")*1000;
    if (worker_params_.socket_encryption_timeout == 0)
    {
        Log::error("Socket encryption timeout is not set or equals to zero");
        exit(1);
    }

    // Timers to handle the situation when the thread pool runs out of available threads
    // and it is continuing for at least 30 seconds, which prevents the server from accepting
    // and processing new requests. From the outside, it looks like the server is hanging and not responding
    QTimer* monitor_timer = new QTimer(this);
    connect(monitor_timer, SIGNAL(timeout()), this,  SLOT(checkPoolStatus()));
    monitor_timer->start(5000); // Check every 5 seconds if we have threads that "got stuck"

    QTimer* reset_email_already_sent_flag_timer = new QTimer(this);
    connect(reset_email_already_sent_flag_timer, SIGNAL(timeout()), this,  SLOT(resetEmailAlreadySentFlag()));
    reset_email_already_sent_flag_timer->start(1000*60*60); // Reseting the flag (a notification about the critial problem with the thread pool) every hour to avoid "spamming" the admins mailboxes
}

SslServer::~SslServer()
{
}

QSslConfiguration SslServer::getSslConfiguration() const
{
	return current_ssl_configuration_;
}

void SslServer::setSslConfiguration(const QSslConfiguration &ssl_configuration)
{
	current_ssl_configuration_ = ssl_configuration;
}

QSslSocket *SslServer::nextPendingConnection()
{
    return static_cast<QSslSocket *>(QTcpServer::nextPendingConnection());
}

void SslServer::resetEmailAlreadySentFlag()
{
    email_already_sent_ = false;
}

void SslServer::checkPoolStatus()
{
    if (email_already_sent_) return;

    int active = thread_pool_.activeThreadCount();
    int max_threads = thread_pool_.maxThreadCount();

    qDebug() << "Active threads: " << active << "/" << max_threads;

    if (active >= max_threads)
    {
        thread_pool_check_count_++;
        if (thread_pool_check_count_ >= 6) // 6 * 5s = 30s
        {
            try
            {
                QStringList admin_emails = Settings::stringList("users_to_be_notified_on_errors", true);
                QString server_email_address = Settings::string("server_email_address", true);
                if (!admin_emails.isEmpty() && !server_email_address.isEmpty())
                {
                    QString thread_pool_error = "Thread pool has been at full capacity for at least 30 seconds!";
                    Log::error(thread_pool_error);

                    QProcess process;
                    QString message = "Dear Admin,\n\nGSvarServer reported the following problem:\n"+thread_pool_error+"\n\nBest regards,\nYour GSvarServer";

                    QStringList cmd_args = {"-f", server_email_address};
                    cmd_args.append(admin_emails);
                    process.start("sendmail", cmd_args);

                    if (!process.waitForStarted()) {
                        Log::error("Failed to start sendmail process for reporting hanging threads");
                    }

                    // Write the message to stdin of sendmail
                    process.write(message.toUtf8());
                    process.closeWriteChannel();
                    email_already_sent_ = true;
                }

            }
            catch (...)
            {
                Log::error("Error while running 'sendmail' command");
            }

            thread_pool_check_count_ = 0;
        }
    } else
    {
        thread_pool_check_count_ = 0;
    }
}

void SslServer::incomingConnection(qintptr socket)
{
    try
    {
        RequestWorker *request_worker = new RequestWorker(current_ssl_configuration_, socket, worker_params_);
        thread_pool_.start(request_worker);
    }
    catch (...)
    {
        Log::error("Unexpected error while processing a client request");
    }
    Log::info("Number of active threads: " + QString::number(thread_pool_.activeThreadCount()) + ", thread pool size: " + QString::number(thread_pool_.maxThreadCount()));
}
