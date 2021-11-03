#include "DescriptorManager.h"

DescriptorManager::DescriptorManager()
	: file_descriptors_()
{
}

DescriptorManager& DescriptorManager::instance()
{
	static DescriptorManager descriptor_manager;
	return descriptor_manager;
}

void DescriptorManager::addNewDescriptor(QString url_id)
{
	instance().mutex_.lock();
	instance().file_descriptors_.insert(url_id, FileDescriptor{QDateTime::currentDateTime(), QSharedPointer<QFile>(new QFile())});
	instance().mutex_.unlock();
}

void DescriptorManager::removeDescriptor(QString url_id)
{
	if (instance().file_descriptors_.contains(url_id))
	{
		instance().mutex_.lock();
		instance().file_descriptors_.remove(url_id);
		instance().mutex_.unlock();
	}
}

bool DescriptorManager::containsDescriptor(QString url_id)
{
	return instance().file_descriptors_.contains(url_id);
}

FileDescriptor DescriptorManager::getDescriptorById(QString url_id)
{
	if (instance().file_descriptors_.contains(url_id))
	{
		return instance().file_descriptors_[url_id];
	}
	return FileDescriptor{};
}
