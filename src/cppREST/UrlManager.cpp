#include "UrlManager.h"
#include "Helper.h"

UrlManager::UrlManager()
	: backup_file_(Helper::openFileForWriting(ServerHelper::getUrlStorageBackupFileName(), false, true))
	, url_storage_()
{
}

UrlManager& UrlManager::instance()
{
	static UrlManager url_manager;
	return url_manager;
}

void UrlManager::saveEverythingToFile()
{
	instance().mutex_.lock();
	QMapIterator<QString, UrlEntity> i(instance().url_storage_);
	QTextStream out(instance().backup_file_.data());

	while (i.hasNext())
	{
		i.next();
		if (!i.value().filename.isEmpty())
		{
			out << i.key() << "\t" << i.value().filename << "\t" << i.value().path << "\t" << i.value().filename_with_path <<
				"\t" << i.value().file_id << "\t" << i.value().created.toString() << "\n";
		}
	}

	instance().mutex_.unlock();
}

void UrlManager::saveUrlToFile(QString id, UrlEntity in)
{
	QTextStream out(instance().backup_file_.data());
	if (!in.isEmpty())
	{
		out << id << "\t" << in.filename << "\t" << in.path << "\t" << in.filename_with_path << "\t" << in.file_id << "\t" << in.created.toSecsSinceEpoch() << "\n";
	}
}

void UrlManager::restoreFromFile()
{
	// the method is not intended to be used when the server is running, since
	// we do not handle concurrency
	if (QFile(ServerHelper::getUrlStorageBackupFileName()).exists())
	{
		int restored_items = 0;
		if (instance().backup_file_.data()->isOpen()) instance().backup_file_.data()->close();
		instance().backup_file_ = Helper::openFileForReading(ServerHelper::getUrlStorageBackupFileName());
		while(!instance().backup_file_.data()->atEnd())
		{
			QString line = instance().backup_file_.data()->readLine();
			if(line.isEmpty()) break;

			QList<QString> line_list = line.split("\t");
			if (line_list.count() > 4)
			{
				bool ok;
				restored_items++;
				addNewUrl(line_list[0], UrlEntity(line_list[1], line_list[2], line_list[3], line_list[4], QDateTime::fromSecsSinceEpoch(line_list[5].toLongLong(&ok,10))), false);
			}
		}
		Log::info("Number of restored URLs: " + QString::number(restored_items));
		instance().backup_file_.data()->close();

		removeExpiredUrls();
		instance().backup_file_ = Helper::openFileForWriting(ServerHelper::getUrlStorageBackupFileName(), false, false);
		QMapIterator<QString, UrlEntity> i(instance().url_storage_);
		while (i.hasNext())
		{
			i.next();
			saveUrlToFile(i.key(), i.value());
		}
		instance().backup_file_ = Helper::openFileForWriting(ServerHelper::getUrlStorageBackupFileName(), false, true);
	}
	else
	{
		Log::info("URL backup has not been found: nothing to restore");
	}
}

void UrlManager::addNewUrl(QString id, UrlEntity in, bool save_to_file)
{
	instance().mutex_.lock();
	instance().url_storage_.insert(id, in);
	if (save_to_file) saveUrlToFile(id, in);
	instance().mutex_.unlock();
}

void UrlManager::removeUrl(const QString& id)
{
	if (instance().url_storage_.contains(id))
	{
		instance().mutex_.lock();
		instance().url_storage_.remove(id);
		instance().mutex_.unlock();
	}
}

bool UrlManager::isInStorageAlready(const QString& filename_with_path)
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

UrlEntity UrlManager::getURLById(const QString& id)
{
	if (instance().url_storage_.contains(id))
	{
		return instance().url_storage_[id];
	}
	return UrlEntity{};
}

void UrlManager::removeExpiredUrls()
{	
	// URL lifetime in seconds
	int url_lifetime = ServerHelper::getNumSettingsValue("url_lifetime");
	if (url_lifetime == 0)
	{
		url_lifetime = 600; // default value, if not set in the config
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
		removeUrl(to_be_removed[i]);
	}
}
