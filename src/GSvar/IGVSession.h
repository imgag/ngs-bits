#ifndef IGVSESSION_H
#define IGVSESSION_H

#include <QThreadPool>
#include <QMutex>

#include "IgvDialog.h"
#include "MainWindow.h"

//Status of a IGV command
enum IGVStatus
{
	QUEUED,
	STARTED,
	FINISHED,
	FAILED
};

//IGV command struct
struct IGVCommand
{
	int id;
	QString command;
	IGVStatus status;
	QString answer;
	double execution_time_sec;
};

//Base data of IGV instance
struct IGVData
{
	QString name;
	QString executable;
	QString host;
	int port;
	QString genome_file;
};

//Keeps information about an individual instance of IGV, allows to run commands, and gives info about their status
class IGVSession
	: public QObject
{
    Q_OBJECT

public:
	IGVSession(QWidget* parent, QString igv_name, QString igv_app, QString igv_host, int igv_port, QString igv_genome);

    //name of a given IGV instance
	const QString getName() const;
	//port number where an IGV instance is running
	int getPort() const;

	//checks if a corresponding instance of IGV app is running
	bool isIgvRunning();
	//checks if the IGV instance is currently executing a command
	bool hasRunningCommands();
	//returns if the session is initializized with tracks of the current analysis
	bool isInitialized() const;
	//sets if the session is initializized with tracks of the current analysis
	void setInitialized(const bool& is_initialized);

	//Queues a list of commands for execution. Starts IGV if not running. Intializes IGV with current GSvar analysis tracks if @init_if_not_done is set to true.
	void execute(const QStringList& commands, bool init_if_not_done);
    //jumps to a specific region in IGV
    void gotoInIGV(const QString& region, bool init_if_not_done);
    //opens a file in IGV using "load" command (in client-server mode a secure token is also set)
    void loadFileInIGV(QString filename, bool init_if_not_done);

	void waitForDone();

	//Executed commands of the current session (since creation or last clear)
	QList<IGVCommand> getCommands() const;
	//Clears the session (unloads tracks, resets init status, clears history);
	void clear();

	static QString statusToString(IGVStatus status);
	static QColor statusToColor(IGVStatus status);

protected:
	//initialize IGV with tracks from current sample (shows dialog for track selection to user). Returns if the user accepted the dialog.
	QStringList initRegularIGV();
	//initialize IGV with tracks from current sample (shows dialog for track selection to user). Returns if the user accepted the dialog.
	QStringList initVirusIGV();
	//converts a list of files to load to a command list
	QStringList filesToCommands(QStringList files_to_load);

signals:
	void historyUpdated(QString, QList<IGVCommand>);
    void started();
    void finished();

private slots:
	void updateHistoryStart(int id);
	void updateHistoryFinished(int id, QString answer, double sec_elapsed);
	void updateHistoryFailed(int id, QString error, double sec_elapsed);

private:
	QWidget* parent_;
    QThreadPool execution_pool_;

	IGVData igv_data_;
	bool is_initialized_;

	int next_id_ = 1; //ID of next command (used to identify commands, e.g. for the history)
	QList<IGVCommand> command_history_;
	mutable QMutex command_history_mutex_;
};

#endif // IGVSESSION_H
