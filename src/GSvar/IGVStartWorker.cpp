#include "IGVStartWorker.h"
#include "Exceptions.h"
#include <QFile>
#include <QProcess>
#include <QTcpSocket>

IGVStartWorker::IGVStartWorker(const QString& igv_host, const int& igv_port, const QString& igv_app)
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

void IGVStartWorker::run()
{
    bool started = QProcess::startDetached(igv_app_ + " --port " + QString::number(igv_port_));
    if (!started)
    {
		emit failed("Could not start IGV: IGV application '" + igv_app_ + "' did not start!");
		return;
    }

    //wait for IGV to respond after start
    bool connected = false;
	for (int i=0; i<30; ++i)
	{
		QTcpSocket socket;
        socket.connectToHost(igv_host_, igv_port_);
        if (socket.waitForConnected(1000))
		{
			connected = true;
            break;
		}
		socket.abort();
    }
    if (!connected)
    {
		emit failed("Could not start IGV: IGV application '" + igv_app_ + "' started, but does not respond on port '" + QString::number(igv_port_) + "'!");
	}
}
