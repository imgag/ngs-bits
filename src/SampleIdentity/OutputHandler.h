#ifndef OUTPUTHANDLER_H
#define OUTPUTHANDLER_H

#include <QFile>
#include <QObject>
#include <QMutex>
#include <QTextStream>
#include <QElapsedTimer>

class OutputHandler : public QObject
{
	Q_OBJECT

public:
	OutputHandler(QTextStream& out_stream, QTextStream& debug_stream);
	void timerStart();

public slots:
	void debugMessage(QString msg);
	void outputMessage(QString msg);
	void bamDone();

private:
	QTextStream& out_stream_;
	QTextStream& debug_stream_;

	QMutex out_stream_mtx_,
		   debug_stream_mtx_,
		   bams_done_mtx_;

	int bams_done_;
	QElapsedTimer timer_;
};

#endif // OUTPUTHANDLER_H
