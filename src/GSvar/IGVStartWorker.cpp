#include "IGVStartWorker.h"
#include "Exceptions.h"
#include <QFile>
#include <QProcess>
#include <QTcpSocket>

IGVStartWorker::IGVStartWorker(QString igv_host, int igv_port, QString igv_app, QString genome_file)
    : igv_host_(igv_host)
    , igv_port_(igv_port)
	, igv_app_(igv_app)
	, genome_(genome_file)
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
	QTcpSocket socket;
	for (int i=0; i<30; ++i) //try up to 30 times
	{
        socket.connectToHost(igv_host_, igv_port_);
        if (socket.waitForConnected(1000))
		{
            break;
		}
		socket.abort();
    }
	if (!socket.isValid())
    {
		emit failed("Could not start IGV: IGV application '" + igv_app_ + "' started on port '" + QString::number(igv_port_) + "', but does not respond !");
	}
	
	//load genome
	socket.write("genome " + genome_.toUtf8() + "\n");
	socket.waitForBytesWritten();
	bool ok = socket.waitForReadyRead(60000); // 1 min timeout ~ takes about 8 sec
	if (!ok)
	{
		emit failed("Could not start IGV: IGV application '" + igv_app_ + "' started on port '" + QString::number(igv_port_) + "', could not load genome!");
	}
}
