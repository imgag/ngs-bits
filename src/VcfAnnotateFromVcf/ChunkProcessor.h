#ifndef CHUNKPROCESSOR_H
#define CHUNKPROCESSOR_H

#include <QRunnable>
#include "Auxilary.h"

class ChunkProcessor
        :public QRunnable
{
public:
	ChunkProcessor(AnalysisJob &job, const QVector<QByteArrayList>& info_id_list, const QVector<QByteArrayList>& out_info_id_list, const QByteArrayList& out_id_column_name_list, const QByteArrayList& annotation_file_list, const QByteArrayList& id_column_name_list, const QVector<bool>& allow_missing_header_list, const QSet<QByteArray>& unique_output_ids, const QByteArrayList& prefix_list);
    void run();

    void terminate()
    {
        terminate_ = true;
    }

private:
    bool terminate_;
	AnalysisJob& job_;
	const QVector<QByteArrayList>& info_id_list_;
	const QVector<QByteArrayList>& out_info_id_list_;
	const QByteArrayList& out_id_column_name_list_;
	const QByteArrayList& annotation_file_list_;
	const QByteArrayList& id_column_name_list_;
	const QVector<bool>& allow_missing_header_list_;
	const QSet<QByteArray>& unique_output_ids_;
	const QByteArrayList& prefix_list_;
};

#endif // CHUNKPROCESSOR_H
