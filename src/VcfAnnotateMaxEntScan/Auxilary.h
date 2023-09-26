#ifndef AUXILARY_H
#define AUXILARY_H

#include <QByteArrayList>
#include <QString>
#include <QVector>
#include <QSet>

#include "Helper.h"
#include "Transcript.h"
#include <QHash>
#include <VcfFile.h>
#include "NGSHelper.h"

//Tool parameters
struct Parameters
{
	QString in;
	QString out;
	QString tag;
	QString tag_swa;
	int decimals;
	float min_score;
	int prefetch;
	int threads;
	int block_size;
	bool debug;
	bool swa;
};

//Analysis data for worker thread
struct AnalysisJob
{
	int index; //job index [0-prefetch] used to identify jobs in slots
	int chunk_nr = -1; //chunk number used to write chunks in input order
	QList<QByteArray> lines;

    void clear()
    {
		chunk_nr = -1;
		lines.clear();
    }
};

//Meta data: annotation file, column names (in and out), etc.
struct MetaData
{
	const QString reference;
	const TranscriptList transcripts;
	const QHash<QByteArray,float> score5_rest_;
	const QHash<int,QHash<int,float>> score3_rest_;
	const QByteArrayList annotation_header_lines;

	MetaData(const QString reference, const TranscriptList transcripts, QHash<QByteArray,float> score5_rest_, QHash<int,QHash<int,float>> score3_rest_, QByteArrayList annotation_header_lines)
		: reference(reference)
		, transcripts(transcripts)
		, score5_rest_(score5_rest_)
		, score3_rest_(score3_rest_)
		, annotation_header_lines(annotation_header_lines)
	{
	}
};

#endif // AUXILARY_H


