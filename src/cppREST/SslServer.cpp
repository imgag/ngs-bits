#include "SslServer.h"

SslServer::SslServer(QObject *parent)
    : QTcpServer(parent)
    , thread_pool_()
{
	current_ssl_configuration_ = QSslConfiguration::defaultConfiguration();
    int thread_timeout = ServerHelper::getNumSettingsValue("thread_timeout")*1000;
    if (thread_timeout == 0)
    {
        Log::error("Thread timeout is not set or equals to zero");
        exit(1);
    }
    thread_pool_.setExpiryTimeout(thread_timeout);
    int thread_count = ServerHelper::getNumSettingsValue("thread_count");
    if (thread_timeout == 0)
    {
        Log::error("Max number of threads is not set or equals to zero");
        exit(1);
    }
    thread_pool_.setMaxThreadCount(thread_count);

    worker_params_.socket_read_timeout = ServerHelper::getNumSettingsValue("socket_read_timeout")*1000;
    if (worker_params_.socket_read_timeout == 0)
    {
        Log::error("Socket reading timeout is not set or equals to zero");
        exit(1);
    }
    worker_params_.socket_write_timeout = ServerHelper::getNumSettingsValue("socket_write_timeout")*1000;
    if (worker_params_.socket_write_timeout == 0)
    {
        Log::error("Socket writing timeout is not set or equals to zero");
        exit(1);
    }
    worker_params_.socket_encryption_timeout = ServerHelper::getNumSettingsValue("socket_encryption_timeout")*1000;
    if (worker_params_.socket_encryption_timeout == 0)
    {
        Log::error("Socket encryption timeout is not set or equals to zero");
        exit(1);
    }
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
