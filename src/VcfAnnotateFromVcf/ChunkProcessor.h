#ifndef CHUNKPROCESSOR_H
#define CHUNKPROCESSOR_H

#include <QObject>
#include <QRunnable>
#include "Auxilary.h"

class ChunkProcessor
	: public QObject
	, public QRunnable
{
	Q_OBJECT

public:
	ChunkProcessor(AnalysisJob &job, const MetaData& meta, Parameters& params);
	~ChunkProcessor();
	virtual void run() override;

signals:
	void done(int i); //signal emitted when job was successful
	void error(int i, QString message); //signal emitted when job failed

private:
	AnalysisJob& job_;
	const MetaData& meta_;
	Parameters& params_;
};

#endif // CHUNKPROCESSOR_H
