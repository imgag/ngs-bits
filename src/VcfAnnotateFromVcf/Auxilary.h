#ifndef AUXILARY_H
#define AUXILARY_H

#include <QByteArrayList>
#include <QString>

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

#endif // AUXILARY_H


