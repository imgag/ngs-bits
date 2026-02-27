#ifndef BAMWORKER_H
#define BAMWORKER_H

#include <QFile>
#include <QRunnable>
#include <QMutex>
#include <Exceptions.h>
#include "VcfFile.h"

typedef QList<signed char> AfData;

struct BamWorkerArgs{
	QString bam;
	AfData& output_af;
	const VcfFile& snps;
	QString ref;

	int min_depth;
	bool debug;
	bool time;
};

class BamWorker
	: public QObject, public QRunnable
{
	Q_OBJECT

public:
	BamWorker(const BamWorkerArgs&);

	void run();

signals:
	void debugMessage(QString);
	void outputMessage(QString);
	void bamDone();

private:
	BamWorkerArgs args_;
};


#endif // BAMWORKER_H
