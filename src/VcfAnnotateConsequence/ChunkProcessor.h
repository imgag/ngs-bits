#ifndef CHUNKPROCESSOR_H
#define CHUNKPROCESSOR_H

#include <QRunnable>
#include <QByteArray>
#include "Auxilary.h"
#include "ChromosomalIndex.h"
#include "Transcript.h"
#include "VariantHgvsAnnotator.h"


class ChunkProcessor
		: public QObject
		, public QRunnable
{
	Q_OBJECT

public:
	ChunkProcessor(AnalysisJob &job, const MetaData& settings, const Parameters& params);
	~ChunkProcessor();
	void run() override;

signals:
	void done(int i); //signal emitted when job was successful
	void error(int i, QString message); //signal emitted when job failed
	void log(int annotated, int skipped);


private:
	QByteArray annotateVcfLine(const QByteArray& line, const ChromosomalIndex<TranscriptList>& transcript_index);
	QByteArray hgvsNomenclatureToString(const QByteArray& allele, const VariantConsequence& hgvs, const Transcript& t);
	static QByteArray csqAllele(const Sequence& ref, const Sequence& alt);

	AnalysisJob& job_;
	const MetaData& settings_;
	const Parameters& params_;
	const FastaFileIndex reference_; // not thread-save -> each creates own copy
	VariantHgvsAnnotator hgvs_anno_;

	int lines_annotated_ = 0;
	int lines_skipped_ = 0;
};

#endif // CHUNKPROCESSOR_H
