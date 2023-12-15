#ifndef BACKGROUNDWORKERBASE_H
#define BACKGROUNDWORKERBASE_H

#include <QObject>
#include <QRunnable>
#include <QDateTime>

//A base class for background workers
class BackgroundWorkerBase
    : public QObject
    , public QRunnable
{
    Q_OBJECT

public:
	BackgroundWorkerBase(QString name);
	QString name();
	void setId(int id);

signals:
	//signal that is emitted before execution
	void started(int id);
	//signal that is emitted when the execution successfully finished
	void finished(int id, int elapsed_ms);
	//signal that is emitted when the execution failed
	void failed(int id, int elapsed_ms, QString error);

protected:
	//Job name
	QString name_;
	//Job id
	int id_;
};

#endif // BACKGROUNDWORKERBASE_H
