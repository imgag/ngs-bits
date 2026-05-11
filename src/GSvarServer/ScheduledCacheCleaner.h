#ifndef SCHEDULEDCACHECLEANER_H
#define SCHEDULEDCACHECLEANER_H

#include <QTimer>
#include <QDateTime>
#include "NGSD.h"

// This class runs a scheduled cleanup taks for the user permissions cache. If we do not do
// that, the server can be using an outdated cache. It may happen, if new samples have been
// added to a project, sample id changes, etc.
class ScheduledCacheCleaner : public QObject
{
public:
	ScheduledCacheCleaner(QObject* parent = nullptr);

private:
	QTimer* timer_;
	NGSD db_;
	void scheduleNextRun();
};

#endif // SCHEDULEDCACHECLEANER_H
