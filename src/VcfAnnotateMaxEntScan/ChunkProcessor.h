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
	QByteArray getInfoHeaderValue(const QByteArray &header_line, QByteArray key);
	int hashseq(const QByteArray& sequence);
	Sequence curate_sequence(const Sequence& sequence);
	bool is_valid_sequence(const Sequence& sequence);
	int base_to_int(const char base);
	QList<Sequence> get_seqs(const Variant& variant, const int& slice_start, const int& slice_end, const int& length, const FastaFileIndex& reference, const Transcript& transcript);
	float score5_consensus(const Sequence& sequence);
	float score5_rest(const Sequence& sequence);
	float score5(const Sequence& sequence);
	float score3(const Sequence& sequence);
	float score3_consensus(const Sequence& sequence);
	float score3_rest(const Sequence& sequence);
	float score_maxent(const Sequence& sequence, float (ChunkProcessor::*scorefunc)(const Sequence&));
	QList<float> get_max_score(const Sequence& context, const float& window_size, float (ChunkProcessor::*scorefunc)(const Sequence&));
	QList<QByteArray> runMES(const Variant& variant, const ChromosomalIndex<TranscriptList>& transcripts, const FastaFileIndex& reference);
	QList<QByteArray> runSWA(const Variant& variant, const ChromosomalIndex<TranscriptList>& transcripts, const FastaFileIndex& reference);
	QByteArray format_score(float score);

	AnalysisJob& job_;
	const MetaData& meta_;
	Parameters& params_;
	const FastaFileIndex reference;

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
