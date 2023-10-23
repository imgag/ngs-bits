#ifndef IGVSTARTWORKER_H
#define IGVSTARTWORKER_H

#include <QRunnable>
#include <QObject>

//A worker class that starts an IGV app and checks if it has been done correctly
class IGVStartWorker
	: public QObject
	, public QRunnable
{
	Q_OBJECT

public:
	IGVStartWorker(QString igv_host, int igv_port, QString igv_app, QString genome_file);
	void run();

signals:
	void failed(QString error);

protected:
    QString igv_host_;
    int igv_port_;
	QString igv_app_;
	QString genome_;
};

#endif // IGVSTARTWORKER_H
