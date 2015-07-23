#ifndef WRITEWORKER_H
#define WRITEWORKER_H

#include <QObject>
#include <QString>
#include <QRunnable>
#include "FastqFileStream.h"

///Fastq Write worker.
class WriteWorker
        : public QRunnable
{
public:
    ///Constructor.
    WriteWorker(FastqOutfileStream* outstream, FastqEntry& outentry);
    void run();

private:
    //input variables
    FastqOutfileStream * outstream_;
    FastqEntry outentry_;

};

#endif


