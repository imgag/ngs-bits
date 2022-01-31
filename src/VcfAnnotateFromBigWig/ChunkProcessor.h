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
	float getAnnotation(const QByteArray&, int start, int end, const QByteArray& ref, const QByteArray& alt);
	
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
	BigWigReader bw_reader;
};

#endif // CHUNKPROCESSOR_H
