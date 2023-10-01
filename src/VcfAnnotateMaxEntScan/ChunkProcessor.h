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
	ChunkProcessor(AnalysisJob &job, const MetaData& meta, const Parameters& params);
	~ChunkProcessor();
	virtual void run() override;

signals:
	void done(int i); //signal emitted when job was successful
	void error(int i, QString message); //signal emitted when job failed

private:
	int hashseq(const QByteArray& sequence);
	int base_to_int(char base);
	void get_seqs(const Variant& variant, int slice_start, int slice_end, int length, const Transcript& transcript, Sequence& ref_seq, Sequence& alt_seq);
	float score5_consensus(const Sequence& sequence);
	float score5_rest(const Sequence& sequence);
	float score5(const Sequence& sequence);
	float score3(const Sequence& sequence);
	float score3_consensus(const Sequence& sequence);
	float score3_rest(const Sequence& sequence);
	float score_maxent(const Sequence& sequence, float (ChunkProcessor::*scorefunc)(const Sequence&));
	QPair<float, int> get_max_score(const Sequence& context, const float& window_size, float (ChunkProcessor::*scorefunc)(const Sequence&));
	QList<QByteArray> runMES(const Variant& variant, const ChromosomalIndex<TranscriptList>& transcripts);
	QList<QByteArray> runSWA(const Variant& variant, const ChromosomalIndex<TranscriptList>& transcripts);
	QByteArray format_score(float score);

	AnalysisJob& job_;
	const MetaData& meta_;
	const Parameters& params_;
	FastaFileIndex reference_;
	QRegExp acgt_regexp_;

	//constants
	QHash<char,float> bgd_ = {
	    {'A',0.27},
	    {'C',0.23},
	    {'G',0.23},
	    {'T',0.27}
	};
	// constants for five prime
	QHash<char,float> cons15_ ={
	    {'A',0.004},
	    {'C',0.0032},
	    {'G',0.9896},
	    {'T',0.0032}
	};
	QHash<char,float> cons25_ ={
	    {'A',0.0034},
	    {'C',0.0039},
	    {'G',0.0042},
	    {'T',0.9884}
	};
	// constants for three prime
	QHash<char,float> cons13_ ={
	    {'A',0.9903},
	    {'C',0.0032},
	    {'G',0.0034},
	    {'T',0.0030}
	};
	QHash<char,float> cons23_ ={
	    {'A',0.0027},
	    {'C',0.0037},
	    {'G',0.9905},
	    {'T',0.0030}
	};
	QHash<Sequence,float> maxent_cache;
};

#endif // CHUNKPROCESSOR_H
