#ifndef THREADCOORDINATOR_H
#define THREADCOORDINATOR_H

#include <QObject>
#include <QThreadPool>
#include <QTextStream>
#include <QSet>
#include "Auxilary.h"

//Coordinator class for chromosome-wise export of germline variats
class ThreadCoordinator
	: public QObject
{
	Q_OBJECT
public:
	ThreadCoordinator(QObject* parent, const ExportParameters& params);
	~ThreadCoordinator();

signals:
	//export done > application can be closed
	void finished();

private slots:
	//Logs a message to stdout
	void log(QString chr, QString message);
	//Informs the coordinator the a chromosome is done
	void done(QString chr);
	//Logs a message to stdout
	void error(QString chr, QString message);

protected:
	void writeGermlineVcf();
	void writeSomaticVcf();
	void exportGeneInformation();

private:
	ExportParameters params_; //not const ref! We need to copy the parameters because the original instance is deleted when the main loop terminates
	SharedData shared_data_;
	QThreadPool thread_pool_;
	QTextStream out_;
	QSet<QString> chrs_done_;
};

#endif // THREADCOORDINATOR_H
