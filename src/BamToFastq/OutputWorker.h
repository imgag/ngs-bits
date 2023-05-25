#ifndef OUTPUTWORKER_H
#define OUTPUTWORKER_H

#include <QRunnable>
#include "FastqFileStream.h"

///Read pair to write.
struct ReadPair
{
	enum Status
	{
		FREE,
		TO_BE_WRITTEN,
		ERROR
	};

	Status status = FREE;
	FastqEntry e1;
	FastqEntry e2;
	QString error_message;
};

struct ReadPairPool
	: public QList<ReadPair>
{
	ReadPairPool(int initial_size);

	ReadPair& nextFreePair();
	void waitAllWritten() const;
};


class OutputWorker
	: public QRunnable
{
public:
	OutputWorker(ReadPairPool& pair_pool, QString out1, QString out2, int compression_level);
	OutputWorker(ReadPairPool& pair_pool, QString out, int compression_level);
	void run();
	void terminate()
	{
		terminate_ = true;
	}

protected:
	bool terminate_;
	ReadPairPool& pair_pool_;
	QSharedPointer<FastqOutfileStream> ostream1_;
	QSharedPointer<FastqOutfileStream> ostream2_;
};

#endif // OUTPUTWORKER_H


