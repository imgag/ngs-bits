#include "IGVCommandWorker.h"

IGVCommandWorker::IGVCommandWorker(const QString& igv_host, int igv_port, const QList<IgvWorkerCommand>& commands, int delay_ms, int max_command_exec_ms)
    : igv_host_(igv_host)
    , igv_port_(igv_port)
	, commands_(commands)
    , delay_ms_(delay_ms)
	, max_command_exec_ms_(max_command_exec_ms)
{
}

void IGVCommandWorker::run()
{
	//Log::info("Prepearing to execute IGV commands (" + QString::number(commands_.count()) + " in total)");
	emit processingStarted();

	//open socket
	QTcpSocket socket;
	socket.connectToHost(igv_host_, igv_port_);
	if (!socket.waitForConnected(max_command_exec_ms_))
	{
		foreach(const IgvWorkerCommand& command, commands_)
		{
			emit commandFailed(command.id, "could not connect to IGV", 0.0);
		}
		emit processingFinished();
		return;
	}

	//wait before first command (IGV bug)
	if (delay_ms_>0)
	{
		//Log::info("Command execution delay is active: " + QString::number(delay_ms_) + " ms");

		QTimer timer;
		timer.setSingleShot(true);
		QEventLoop loop;
		connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);
		timer.start(delay_ms_);
		loop.exec();
	}

	foreach(const IgvWorkerCommand& command, commands_)
	{
		try
		{
			//Log::info("Executing IGV command: " + command.text);
			emit commandStarted(command.id);

			QTime timer;
			timer.start();

			socket.write((command.text + "\n").toUtf8());
			socket.waitForBytesWritten();
			bool ok = socket.waitForReadyRead(max_command_exec_ms_); // 3 min timeout (trios can be slow)
			if (!ok)
			{
				emit commandFailed(command.id, "command timed out", (double)(timer.elapsed())/1000.0);
				continue;
			}

			answer_ = socket.readAll().trimmed();
			//Log::info(answer);
			if (answer_.startsWith("error", Qt::CaseInsensitive) || answer_.startsWith("unkown command", Qt::CaseInsensitive))
			{
				emit commandFailed(command.id, answer_, (double)(timer.elapsed())/1000.0);
			}
			else
			{
				//Log::info("'" + command + "' - Done");
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
