#include "IGVCommandWorker.h"

#include <QProcess>
#include <QThread>

IGVCommandWorker::IGVCommandWorker(const IGVData& igv_data, const QList<IgvWorkerCommand>& commands, int max_command_exec_ms)
	: igv_data_(igv_data)
	, commands_(commands)
	, max_command_exec_ms_(max_command_exec_ms)
{
}

void IGVCommandWorker::run()
{
	//Log::info("Prepearing to execute IGV commands (" + QString::number(commands_.count()) + " in total)");
	emit processingStarted();

	//special handling of first command if it is "launch IGV"
	int launch_command_id = -1;
	if (!commands_.isEmpty() && commands_.at(0).text=="launch IGV")
	{
        QElapsedTimer timer;
		timer.start();

		launch_command_id = commands_.at(0).id;

		emit commandStarted(launch_command_id);

		//start IGV
        bool started = QProcess::startDetached(igv_data_.executable, QStringList() << "--port" << QString::number(igv_data_.port));
		if (!started)
		{
			emit commandFailed(launch_command_id, "Could not start IGV: IGV application '" + igv_data_.executable + "' did not start!", (double)(timer.elapsed())/1000.0);
		}
		else
		{
			//wait for IGV to respond after start
			QTcpSocket socket;
			for (int i=0; i<30; ++i) //try up to 30 times, i.e. 30 secs
			{
				socket.connectToHost(igv_data_.host, igv_data_.port);
				if (socket.waitForConnected(1000))
				{
					break;
				}
				socket.abort();
			}
			if (!socket.isValid())
			{
				emit commandFailed(launch_command_id, "Could not start IGV: IGV application '" + igv_data_.executable + "' started on port '" + QString::number(igv_data_.port) + "', but does not respond!", (double)(timer.elapsed())/1000.0);
			}

            //wait 5s until the genome is loaded (currently loaded genome is not detected correctly and consistently, possible concurrency issue in IGV)
			QThread::msleep(5000);

			emit commandFinished(launch_command_id, answer_, (double)(timer.elapsed())/1000.0);
		}
	}

	//open socket
	QTcpSocket socket;
	socket.connectToHost(igv_data_.host, igv_data_.port);

	//if no socket connection is possible > mark all commands as failed and abort
	if (!socket.waitForConnected(max_command_exec_ms_))
	{
		foreach(const IgvWorkerCommand& command, commands_)
		{
			emit commandFailed(command.id, "could not connect to IGV", 0.0);
		}
		emit processingFinished();
		return;
	}

	//execute all commands
	foreach(const IgvWorkerCommand& command, commands_)
	{
		try
		{
			//skip all launch commands
			if (command.text=="launch IGV")
			{
				if (command.id!=launch_command_id) //flag secondary launch commands as skipped
				{
					emit commandStarted(command.id);
					emit commandFinished(command.id, "Skipped - IGV already running", 0.0);
				}
				continue;
			}

            QElapsedTimer timer;
			timer.start();

			emit commandStarted(command.id);

			socket.write((command.text + "\n").toUtf8());
			socket.waitForBytesWritten();
			bool ok = socket.waitForReadyRead(max_command_exec_ms_); // 3 min timeout (trios can be slow)
			if (!ok)
			{
				emit commandFailed(command.id, "command timed out", (double)(timer.elapsed())/1000.0);
				continue;
			}

			answer_ = socket.readAll().trimmed();
			if (answer_.startsWith("error", Qt::CaseInsensitive) || answer_.startsWith("unkown command", Qt::CaseInsensitive))
			{
				emit commandFailed(command.id, answer_, (double)(timer.elapsed())/1000.0);
			}
			else
			{
				emit commandFinished(command.id, answer_, (double)(timer.elapsed())/1000.0);
			}
		}
		catch (...)
		{
			emit commandFailed(command.id, "Unknown error while executing IGV commands", 0.0);
		}
	}

	socket.disconnectFromHost();

	emit processingFinished();
}

const QString& IGVCommandWorker::answer() const
{
	return answer_;
}
