#ifndef IGVSESSION_H
#define IGVSESSION_H

#include <QThreadPool>
#include <QAbstractSocket>
#include <QFile>
#include <QProcess>
#include <QEventLoop>
#include <QMessageBox>
#include <QMutex>

#include "Exceptions.h"
#include "Settings.h"
#include "IGVCommandWorker.h"
#include "IGVStartWorker.h"
#include "IgvDialog.h"
#include "ClientHelper.h"
#include "GSvarHelper.h"
#include "LoginManager.h"
#include "GlobalServiceProvider.h"
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

//Keeps information about an individual instance of IGV, allows to run commands, and gives info about their status
class IGVSession
	: public QObject
{
    Q_OBJECT

public:
    IGVSession(QWidget *parent, Ui::MainWindow parent_ui, QString igv_name, QString igv_app, QString igv_host, int igv_port, QString igv_genome);

    //name of a given IGV instance
    const QString getName();
    //define a name for a specific IGV instance
    void setName(const QString& name);
    //host where an IGV instance is running
    const QString getHost();
    //define a host for an IGV instance
    void setHost(const QString& host);
    //port number where an IGV instance is running
    int getPort();
    //define a port number where an IGV instance is running
    void setPort(const int& port);
    //reference genome that is used for an IGV instance
    const QString getGenome();
    //define a reference genome to be loaded for an IGV instance
    void setGenome(const QString& genome);
    //sets a flag indicating that a list of files for IGV has been defined
    void setIGVInitialized(const bool& is_initialized);
    //checks if the user has selected files to be loaded into IGV
    bool isIGVInitialized();
    //launches and initializes IGV app
    void initIGV();
    //sends a list of commands to IGV
    void execute(const QStringList& commands);
    //checks if an instance of IGV has been started and initialized, and if yes, a list of commands is executed
    void prepareIfNotAndExecute(const QStringList& commands, bool init_if_not_done);
    //checks if an instance of IGV has been started and initialized, and if yes, a single command is executed
    void prepareIfNotAndExecuteSingle(const QString& command, bool init_if_not_done);
    //jumps to a specific region in IGV
    void gotoInIGV(const QString& region, bool init_if_not_done);
    //opens a file in IGV using "load" command (in client-server mode a secure token is also set)
    void loadFileInIGV(QString filename, bool init_if_not_done);
    //checks if a corresponding instance of IGV app is running
    bool isIgvRunning();
    //checks if an IGV instance is currently executing a command
    bool hasRunningCommands();
    //contains a list of errors for all commands (each item in the list corresponds to an item in the list of commands)
	QStringList getErrorMessages();
    //history of executed commands (for the current session)
	QList<IGVCommand> getHistory();
    //removes the history of executed commands
    void clearHistory();

	static QString statusToString(IGVStatus status);
	static QColor statusToColor(IGVStatus status);

protected:
    QString getCurrentFileName();
    AnalysisType getCurrentAnalysisType();
    void displayIgvHistoryButton(bool visible);
    QString germlineReportSample();
    bool prepareRegularIGV();
    bool prepareVirusIGV();
    void prepareAndRunIGVCommands(QStringList files_to_load);
    bool igvDialogButtonHandler(IgvDialog& dlg);

signals:
	void historyUpdated(QList<IGVCommand> updated_history);
    void started();
    void finished();

public slots:
	void updateHistoryStart(int id);
	void updateHistoryFinished(int id, QString answer, double sec_elapsed);
	void updateHistoryFailed(int id, QString error, double sec_elapsed);

private:
	QWidget* parent_;
    Ui::MainWindow parent_ui_;
    QThreadPool execution_pool_;

    QString igv_name_;
    QString igv_app_;
    QString igv_host_;
    int igv_port_;
    QString igv_genome_;

    bool is_initialized_;
	bool is_igv_running_;
    QStringList error_messages_;
	int next_id_ = 1;
	QList<IGVCommand> command_history_;
	QMutex command_history_mutex_;
};

#endif // IGVSESSION_H
