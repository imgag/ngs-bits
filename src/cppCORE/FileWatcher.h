#ifndef FILEWATCHER_H
#define FILEWATCHER_H

#include "cppCORE_global.h"
#include <QFileSystemWatcher>
#include <QTimer>

/// File watcher with a delayed trigger (default delay is 1.0 sec)
class CPPCORESHARED_EXPORT FileWatcher
		:  public QObject
{
	Q_OBJECT

public:
	FileWatcher(QObject* parent = 0);
	void setDelayInSeconds(double delay);
	void setFile(QString file);
	void clearFile();

signals:
	void fileChanged();

protected:
	QFileSystemWatcher watcher_;
	QTimer timer_;
};


#endif
