#ifndef CHUNKPROCESSOR_H
#define CHUNKPROCESSOR_H

#include <QRunnable>
#include <QByteArray>
#include "Auxilary.h"

class ChunkProcessor
		:public QRunnable
{
public:
	ChunkProcessor(AnalysisJob &job_, const QByteArray& name_, const QByteArray& desc_, const QByteArray& bw_filepath_);
	void run();

	void terminate()
	{
		terminate_ = true;
	}

private:
	bool terminate_;
	AnalysisJob& job;
	const QByteArray name;
	const QByteArray desc;
	const QByteArray bw_filepath;
};

#endif // CHUNKPROCESSOR_H
