#include "UrlManager.h"

UrlManager::UrlManager()
	: url_storage_()
{
}

UrlManager& UrlManager::instance()
{
	static UrlManager url_manager;
	return url_manager;
}

void UrlManager::addUrlToStorage(QString id, QString filename, QString path, QString filename_with_path)
{
	instance().mutex_.lock();
	instance().url_storage_.insert(id, UrlEntity{filename, path, filename_with_path, id, QDateTime::currentDateTime()});
	instance().mutex_.unlock();
}

void UrlManager::removeUrlFromStorage(QString id)
{
	if (instance().url_storage_.contains(id))
	{
		instance().mutex_.lock();
		instance().url_storage_.remove(id);
		instance().mutex_.unlock();
	}
}

bool UrlManager::isInStorageAlready(QString filename_with_path)
{
	QMapIterator<QString, UrlEntity> i(instance().url_storage_);
	while (i.hasNext()) {
		i.next();		
		if (i.value().filename_with_path == filename_with_path)
		{
			return true;
		}
	}

	return false;
}

UrlEntity UrlManager::getURLById(QString id)
{
	if (instance().url_storage_.contains(id))
	{
		return instance().url_storage_[id];
	}
	return UrlEntity{};
}

void UrlManager::removeExpiredUrls()
{	
	int url_lifetime = ServerHelper::getNumSettingsValue("url_lifetime");
	if (url_lifetime == 0)
	{
		url_lifetime = 1;
	}

	QList<QString> to_be_removed {};
	QMapIterator<QString, UrlEntity> i(instance().url_storage_);
	while (i.hasNext()) {
		i.next();
		int lifetime = (QDateTime::currentDateTime().toSecsSinceEpoch() - i.value().created.toSecsSinceEpoch()) / 60;		
		if (lifetime >= url_lifetime)
		{
			to_be_removed.append(i.key());
		}
	}

	for (int i = 0; i < to_be_removed.count(); ++i)
	{
		removeUrlFromStorage(to_be_removed[i]);
	}
}
