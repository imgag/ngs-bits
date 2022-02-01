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
	ChunkProcessor(AnalysisJob &job, const QByteArray& name, const QByteArray& desc, const QByteArray& bw_filepath);
	void run();
    QList<float> getAnnotation(const QByteArray& chr, int start, int end, const QString& ref, const QString& alt);

	void terminate()
	{
		terminate_ = true;
	}

private:

	QList<float> interpretIntervals(const QList<BigWigReader::OverlappingInterval>& intervals);
	bool terminate_;
	AnalysisJob& job_;
	const QByteArray name_;
	const QByteArray desc_;
	const QByteArray bw_filepath_;
	BigWigReader bw_reader_;
};

#endif // CHUNKPROCESSOR_H
