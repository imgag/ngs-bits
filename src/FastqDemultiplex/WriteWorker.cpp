#include "WriteWorker.h"

WriteWorker::WriteWorker(FastqOutfileStream* outstream, FastqEntry& outentry)
    : QRunnable()
    , outstream_(outstream)
    , outentry_(outentry)
{
}

void WriteWorker::run()
{
    outstream_->write(outentry_);
}
