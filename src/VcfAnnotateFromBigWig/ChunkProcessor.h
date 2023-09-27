#ifndef CHUNKPROCESSOR_H
#define CHUNKPROCESSOR_H

#include <QRunnable>
#include <QByteArray>
#include "Auxilary.h"
#include "BigWigReader.h"

class ChunkProcessor
		:public QRunnable
{
public:
	ChunkProcessor(AnalysisJob &job, const QByteArray& name, const QByteArray& bw_filepath, const QString& modus);
	void run();
	QList<float> getAnnotation(const QByteArray& chr, int start, int end, const QByteArray& ref, const QByteArray& alt);

	void terminate()
	{
		terminate_ = true;
	}

private:

	QList<float> interpretIntervals(const QList<BigWigReader::OverlappingInterval>& intervals, int start, int end);
	bool terminate_;
	AnalysisJob& job_;
	const QByteArray name_;
	const QByteArray bw_filepath_;
	BigWigReader bw_reader_;
	const QString modus_;
};

#endif // CHUNKPROCESSOR_H
