#ifndef FASTQWRITER_H
#define FASTQWRITER_H

#include <QRunnable>
#include "Auxilary.h"

//Output worker
class FastqWriter
	: public QObject
	, public QRunnable
{
	Q_OBJECT

public:
	FastqWriter(const AnalysisJob& job, OutputStreams& streams, const TrimmingParameters& params, bool r1);
	virtual ~FastqWriter();
	virtual void run() override;

private:
	const AnalysisJob& job_;
	OutputStreams& streams_;
	const TrimmingParameters& params_;
	bool r1_;
};

#endif // FASTQWRITER_H


