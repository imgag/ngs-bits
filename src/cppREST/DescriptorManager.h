#ifndef DESCRIPTORMANAGER_H
#define DESCRIPTORMANAGER_H

#include "cppREST_global.h"
#include <QMap>
#include <QDebug>
#include <QMutex>
#include <QDateTime>
#include "ServerHelper.h"

struct CPPRESTSHARED_EXPORT FileDescriptor
{
	QDateTime created;
	QSharedPointer<QFile> file;
};

class CPPRESTSHARED_EXPORT DescriptorManager
{
public:
	static void addNewDescriptor(QString url_id);
	static void removeDescriptor(QString url_id);
	static bool containsDescriptor(QString url_id);
	static FileDescriptor getDescriptorById(QString url_id);

protected:
	DescriptorManager();

private:
	static DescriptorManager& instance();
	QMutex mutex_;
	QMap<QString, FileDescriptor> file_descriptors_;
};

#endif // DESCRIPTORMANAGER_H
