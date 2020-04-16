#ifndef CHUNKPROCESSOR_H
#define CHUNKPROCESSOR_H

#include <QRunnable>
#include <QByteArray>
#include <QByteArrayList>
#include <QTextStream>
#include <iostream>
#include <QBuffer>
#include <QVector>
#include <Auxilary.h>
#include <Exceptions.h>
#include <TabixIndexedFile.h>
#include <QMutex>
#include <Helper.h>

class ChunkProcessor
        :public QRunnable
{
public:
    ChunkProcessor(AnalysisJob &job,
                   QByteArrayList &prefix_list,
                   QSet<QByteArray> &ids,
                   const QVector<QByteArrayList> &info_id_list,
                   const QVector<QByteArrayList> &out_info_id_list,
                   const QByteArrayList &out_id_column_name_list,
                   const QByteArrayList &id_column_name_list,
                   const QVector<bool> &allow_missing_header_list,
                   QByteArrayList &annotation_file_list,
                   const QString &outpath,
                   const QString &input_path

                   );
    void run();

    void terminate()
    {
        terminate_ = true;
    }

private:
    bool terminate_;
    AnalysisJob &job;
    QByteArrayList & prefix_list;
    QSet<QByteArray> &ids;
    const QVector<QByteArrayList> &info_id_list;
    const QVector<QByteArrayList> &out_info_id_list;
    const QByteArrayList &out_id_column_name_list;
    const QByteArrayList &id_column_name_list;
    const QVector<bool> &allow_missing_header_list;
    QByteArrayList &annotation_file_list;
    const QString &outpath;
    const QString &input_path;
};

#endif // CHUNKPROCESSOR_H
