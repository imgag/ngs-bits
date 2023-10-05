#include "IGVCommandWorker.h"

IGVCommandWorker::IGVCommandWorker(const QString& igv_host, const int& igv_port, const QStringList& commands, QStringList &responses, int delay_ms)
    : igv_host_(igv_host)
    , igv_port_(igv_port)
    , commands_(commands)
    , responses_(responses)
    , delay_ms_(delay_ms)
{
}

void IGVCommandWorker::run()
{
    emit commandQueueStarted();
    try
    {
        QTcpSocket socket;
        responses_.clear();

        Log::info("Prepearing to execute IGV commands (" + QString::number(commands_.count()) + " in total)");

        socket.connectToHost(igv_host_, igv_port_);
        if (socket.waitForConnected(10000))
        {
            if (delay_ms_>0)
            {
                Log::info("Command execution delay is active: " + QString::number(delay_ms_) + " ms");

                QTimer timer;
                timer.setSingleShot(true);
                QEventLoop loop;
                connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);
                timer.start(delay_ms_);
                loop.exec();
            }

            QString answer;
            foreach(QString command, commands_)
            {
                Log::info("Executing IGV command: " + command);
                emit commandReported(QDateTime::currentDateTime().toString("MMMM d hh:mm:ss") + "\t'" + command + "'\tstarted");

                socket.write((command + "\n").toUtf8());
                socket.waitForBytesWritten();
                bool ok = socket.waitForReadyRead(180000); // 3 min timeout (trios can be slow)
                if (!ok)
                {
                    emit commandFailed("IGV command '" + command + "' timed out");
                    emit commandReported(QDateTime::currentDateTime().toString("MMMM d hh:mm:ss") + "\t'" + command + "'\ttimed out");
                }

                answer = socket.readAll().trimmed();
                responses_ << answer;

                Log::info(answer);
                if (answer.toLower().startsWith("error") || answer.toLower().startsWith("unkown command"))
                {
                    emit commandFailed("Could not execute IGV command '" + command + "'.\nAnswer: " + answer + "\nSocket error:" + socket.errorString());
                    emit commandReported(QDateTime::currentDateTime().toString("MMMM d hh:mm:ss") + "\t'" + command + "'\tfailed");
                }
                else
                {
                    Log::info("'" + command + "' - Done");
                    emit commandReported(QDateTime::currentDateTime().toString("MMMM d hh:mm:ss") + "\t'" + command + "'\tfinished");
                }
            }
        }

        socket.disconnectFromHost();
    }
    catch (...)
    {
        emit commandFailed("Unknown error while executing IGV commands");
        emit commandReported(QDateTime::currentDateTime().toString("MMMM d hh:mm:ss") + "\t---\tunknown error");
    }
    emit commandQueueFinished();
}
