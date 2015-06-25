#include "FileWatcher.h"
#include <QTimer>
#include <QStringList>

FileWatcher::FileWatcher(QObject* parent)
	: QObject(parent)
	, watcher_()
	, timer_()
{
	timer_.setSingleShot(true);
	timer_.setInterval(1000);
	connect(&watcher_, SIGNAL(fileChanged(QString)), &timer_, SLOT(start()));
	connect(&timer_, SIGNAL(timeout()), this, SIGNAL(fileChanged()));
}

void FileWatcher::setDelayInSeconds(double delay)
{
	timer_.setInterval((int)(1000.0 * delay));
}

void FileWatcher::setFile(QString file)
{
	clearFile();
	watcher_.addPath(file);
}

void FileWatcher::clearFile()
{
	timer_.stop();
	if (watcher_.files().count()!=0)
	{
		watcher_.removePaths(watcher_.files());
	}
}
