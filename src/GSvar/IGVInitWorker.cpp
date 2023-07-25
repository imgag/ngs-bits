#include "IGVInitWorker.h"

IGVInitWorker::IGVInitWorker(const QString& igv_host, const int& igv_port, const QString& igv_app)
    : igv_host_(igv_host)
    , igv_port_(igv_port)
    , igv_app_(igv_app)
{
    if (igv_app_.isEmpty())
    {
        THROW(Exception, "Could not start IGV: No settings entry for 'igv_app' found!");
    }
    if (!QFile::exists(igv_app))
    {
        THROW(Exception, "Could not start IGV: IGV application '" + igv_app_ + "' does not exist!");
    }
}

void IGVInitWorker::run()
{
    bool started = QProcess::startDetached(igv_app_ + " --port " + QString::number(igv_port_));
    if (!started)
    {
        THROW(Exception, "Could not start IGV: IGV application '" + igv_app_ + "' did not start!");
    }

    QTcpSocket socket;
    //wait for IGV to respond after start
    bool connected = false;
    QDateTime max_wait = QDateTime::currentDateTime().addSecs(40);
    while (QDateTime::currentDateTime() < max_wait)
    {
        socket.connectToHost(igv_host_, igv_port_);
        if (socket.waitForConnected(1000))
        {
            Log::info("Connecting to the IGV ports works");
            connected = true;
            break;
        }
    }
    if (!connected)
    {
        THROW(Exception, "Could not start IGV: IGV application '" + igv_app_ + "' started, but does not respond!");
    }
    socket.disconnectFromHost();
}
