#include "SgeStatusUpdateWorker.h"

SgeStatusUpdateWorker::SgeStatusUpdateWorker()
    : QRunnable()
{

}

void SgeStatusUpdateWorker::run()
{
    Log::info("SGE status update worker is running");
}
