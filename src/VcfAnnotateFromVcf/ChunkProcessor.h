#ifndef CHUNKPROCESSOR_H
#define CHUNKPROCESSOR_H

#include <QRunnable>
#include "Auxilary.h"

class ChunkProcessor
        :public QRunnable
{
public:
	ChunkProcessor(AnalysisJob &job, const MetaData& meta);
    void run();

    void terminate()
    {
        terminate_ = true;
    }

private:
    bool terminate_;
	AnalysisJob& job_;
	const MetaData& meta_;
};

#endif // CHUNKPROCESSOR_H
