#ifndef AUXILARY_H
#define AUXILARY_H

#include <QByteArrayList>
#include <QString>
#include <QVector>
#include <QSet>

//Analysis status
enum AnalysisStatus
{
    TO_BE_PROCESSED,
    TO_BE_WRITTEN,
	ERROR,
	DONE
};

//Analysis data for worker thread
struct AnalysisJob
{
	QList<QByteArray> lines;
    QString error_message;

    //id used to keep the vcf file in order
    int chunk_id = 0;
    AnalysisStatus status = DONE;

    void clear()
    {
		lines.clear();
		error_message.clear();
        status = DONE;
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
	QVector<bool> allow_missing_header_list;
	QSet<QByteArray> unique_output_ids;
	QByteArrayList prefix_list;
};

#endif // AUXILARY_H


