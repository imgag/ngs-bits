#include "UrlManager.h"
#include "Helper.h"

UrlManager::UrlManager()
    : url_storage_()
{
}

UrlManager& UrlManager::instance()
{
	static UrlManager url_manager;
	return url_manager;
}

void UrlManager::restoreFromFile()
{
	// the method is not intended to be used when the server is running, since
	// we do not handle concurrency
	if (QFile(ServerHelper::getUrlStorageBackupFileName()).exists())
	{
        qint64 restored_items = 0;
        QSharedPointer<QFile> backup_file = Helper::openFileForReading(ServerHelper::getUrlStorageBackupFileName());
        while(!backup_file.data()->atEnd())
		{
            QString line = backup_file.data()->readLine();
			if(line.isEmpty()) break;

			QList<QString> line_list = line.split("\t");
			if (line_list.count() > 4)
			{
				bool ok;

                UrlEntity current_url = UrlEntity(line_list[1], line_list[2], line_list[3], line_list[4], QDateTime::fromSecsSinceEpoch(line_list[5].toLongLong(&ok,10)));
                if (!isUrlExpired(current_url))
                {
                    restored_items++;
                    addNewUrl(line_list[0], current_url);
                }
			}
		}
		Log::info("Number of restored URLs: " + QString::number(restored_items));
        backup_file.data()->close();
	}
	else
	{
		Log::info("URL backup has not been found: nothing to restore");
	}
}

void UrlManager::addNewUrl(QString id, UrlEntity in)
{
	instance().mutex_.lock();
	instance().url_storage_.insert(id, in);	
	instance().mutex_.unlock();
}

void UrlManager::removeUrl(const QString& id)
{
    instance().mutex_.lock();
    if (instance().url_storage_.contains(id))
    {
        instance().url_storage_.remove(id);
    }
    instance().mutex_.unlock();
}

bool UrlManager::isInStorageAlready(const QString& filename_with_path)
{
    bool is_found = false;
    instance().mutex_.lock();
    QMapIterator<QString, UrlEntity> i(instance().url_storage_);
    while (i.hasNext())
    {
		i.next();		
		if (i.value().filename_with_path == filename_with_path)
		{
            is_found = true;
		}
	}
    instance().mutex_.unlock();

    return is_found;
}

UrlEntity UrlManager::getURLById(const QString& id)
{
    UrlEntity found_entity = {};
    instance().mutex_.lock();
    if (instance().url_storage_.contains(id))
    {
        found_entity = instance().url_storage_[id];
	}
    instance().mutex_.lock();
    return found_entity;
}

bool UrlManager::isUrlExpired(UrlEntity in)
{
    // URL lifetime in seconds
    int url_lifetime = ServerHelper::getNumSettingsValue("url_lifetime");
    if (url_lifetime == 0)
    {
        url_lifetime = 600; // default value, if not set in the config
    }
    if ((QDateTime::currentDateTime().toSecsSinceEpoch() - in.created.toSecsSinceEpoch()) >= url_lifetime)
    {
        return true;
    }
    return false;
}

QMap<QString, UrlEntity> UrlManager::removeExpiredUrls()
{
    QMap<QString, UrlEntity> tmp_storage = {};
    QList<QString> to_be_removed {};

    Log::info("Starting to cleanup URLs");
    instance().mutex_.lock();
	QMapIterator<QString, UrlEntity> i(instance().url_storage_);
    while (i.hasNext())
    {
		i.next();        
        if (isUrlExpired(i.value()))
        {
			to_be_removed.append(i.key());
		}
	}    
    for (int i = 0; i < to_be_removed.count(); ++i)
    {
        instance().url_storage_.remove(to_be_removed[i]);
    }
    tmp_storage = instance().url_storage_;
    instance().mutex_.unlock();

    Log::info("Number of removed URLs: " + QString::number(to_be_removed.length()));
    return tmp_storage;
}
