#ifndef IGVCOMMANDWORKER_H
#define IGVCOMMANDWORKER_H

#include <QRunnable>

#include "IGVSession.h"

//Datastructure for IGV commands with ID.
struct IgvWorkerCommand
{
	int id;
	QString text;
};

//A worker class for running IGV commands. Commands are executed one after another and do not run
//in parallel. Status and errors are reported by emited signals and inside "responses" variable
class IGVCommandWorker
    : public QObject
    , public QRunnable
{
    Q_OBJECT

public:
	IGVCommandWorker(const IGVData& igv_data, const QList<IgvWorkerCommand>& commands, int max_command_exec_ms=180000);
	void run();
	const QString& answer() const;

signals:
	//signal that is emitted before the first command is executed
	void processingStarted();
	//signal that is emitted after the last command is executed
	void processingFinished();

	//signal that is emitted when the execution of a command begins
	void commandStarted(int id);
	//signal that is emitted when the execution of a command was successful
	void commandFinished(int id, QString response, double sec_elapsed);
	//signal that is emitted when the execution of a command failed
	void commandFailed(int id, QString error, double sec_elapsed);

protected:   
	const IGVData& igv_data_;
	QList<IgvWorkerCommand> commands_;
	int max_command_exec_ms_;

	QString answer_;
};

#endif // IGVCOMMANDWORKER_H
