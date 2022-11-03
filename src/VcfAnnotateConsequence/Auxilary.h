#ifndef AUXILARY_H
#define AUXILARY_H

#include <QByteArray>
#include <QString>
#include "ChromosomalIndex.h"
#include "Transcript.h"


//Tool parameters
struct Parameters
{
	QString in;
	QString out;
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
	int max_dist_to_trans;
	int splice_region_ex;
	int splice_region_in_5;
	int splice_region_in_3;

	MetaData(const QByteArray tag, const QString reference, TranscriptList transcripts)
		: tag(tag)
		, reference(reference)
		, transcripts(transcripts)
	{
	}


};

#endif // AUXILARY_H
