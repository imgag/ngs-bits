#include "ScheduledCacheCleaner.h"
#include "Log.h"

ScheduledCacheCleaner::ScheduledCacheCleaner(QObject *parent)
	: QObject(parent)
	, db_(NGSD())
{
	timer_ = new QTimer(this);
	timer_->setSingleShot(true);

	connect(timer_, &QTimer::timeout, this, &ScheduledCacheCleaner::scheduleNextRun);

	// Initial scheduling
	Log::info("Initial start of the scheduled permissions cache cleanup");
	scheduleNextRun();
}

void ScheduledCacheCleaner::scheduleNextRun()
{
	Log::info("Permissions cache cleaning...");
	db_.clearUserPermissionsCache();

	// Runs every day at 3 a.m.
	QDateTime now = QDateTime::currentDateTime();
	QDateTime next = QDateTime(now.date(), QTime(3, 0));

	if (now >= next) next = next.addDays(1);

	qint64 delay = now.msecsTo(next);

	Log::info("Next run at: " + next.toString());
	Log::info("Delay (seconds): " + QString::number(delay/1000));

	timer_->start(delay);
}
