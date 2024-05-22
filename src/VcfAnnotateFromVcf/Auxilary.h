#ifndef AUXILARY_H
#define AUXILARY_H

#include <QByteArrayList>
#include <QString>
#include <QVector>
#include <QSet>

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
	QVector<QByteArrayList> info_id_list;
	QVector<QByteArrayList> out_info_id_list;
	QByteArrayList out_id_column_name_list;
	QByteArrayList annotation_file_list;
	QByteArrayList id_column_name_list;
	QVector<bool> annotate_only_existence;
	QByteArrayList existence_name_list;
	QVector<bool> allow_missing_header_list;
	QSet<QByteArray> unique_output_ids;
	QByteArrayList prefix_list;
};

#endif // AUXILARY_H


