#ifndef AUXILARY_H
#define AUXILARY_H

#include "Transcript.h"
#include "VariantHgvsAnnotator.h"


//Tool parameters
struct Parameters
{
	QString in;
	QString out;
	QString source;
	int prefetch;
	int threads;
	int block_size;
	bool debug;
};

//Analysis data for worker thread
struct AnalysisJob
{
	int index; //job index [0-prefetch] used to identify jobs in slots
	int chunk_nr = -1; //chunk number used to write chunks in input order
	QByteArrayList lines;

	void clear()
	{
		chunk_nr = -1;
		lines.clear();
	}
};

//Meta data: infos needed for annotation
struct MetaData
{
	const QByteArray tag;
	const QString reference;
	const TranscriptList transcripts;
	VariantHgvsAnnotator::Parameters annotation_parameters;

	MetaData(const QByteArray tag, const QString reference, TranscriptList transcripts)
		: tag(tag)
		, reference(reference)
		, transcripts(transcripts)
	{
	}
};

#endif // AUXILARY_H
