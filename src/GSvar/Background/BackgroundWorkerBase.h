#ifndef BACKGROUNDWORKERBASE_H
#define BACKGROUNDWORKERBASE_H

#include <QObject>
#include <QRunnable>

//A base class for background workers. Implement process() and start in BackgroundJobDialog.
//Attention: background workers are not auto-deleted. Start with BackgroundJobDialog or take care of deleting them yourself.
class BackgroundWorkerBase
    : public QObject
    , public QRunnable
{
    Q_OBJECT

public:
	BackgroundWorkerBase(QString name);
	//Returns the job name
	QString name();
	//Sets the job ID
	void setId(int id);
	//Returns the job ID
	int id() const;
	//Returns the milliseconds elapsed while processing
	int elapsed() const;
	//Returns the error message. If not set, the job terminated successfully.
	const QString& error() const;

	//Re-implement this method for the actual processing
	virtual void process() = 0;
	//Re-implement this method if user-interation after the processing is required. The default implementation is empty
	virtual void userInteration();

	//Re-implements the QRunnable::run method to perform error handling, runtime measurement, etc.
	//Attention: do not override yourself!!
	void run() override;

signals:
	//signal that is emitted before execution
	void started();
	//signal that is emitted when the execution successfully finished
	void finished();
	//signal that is emitted when the execution failed
	void failed();

protected:
	//Job name
	QString name_;
	//Job id
	int id_;
	//ms elapsed for processing
	int elapsed_ms_;
	//error message. If not set, the job terminated successfully.
	QString error_;
};

#endif // BACKGROUNDWORKERBASE_H
