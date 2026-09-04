#include "UrlManager.h"
#include "Settings.h"
#include "Exceptions.h"
#include "Log.h"
#include "NGSD.h"

UrlManager::UrlManager()
	: url_storage_()
	, current_url_lifetime_(0)
{
	try
	{
		current_url_lifetime_ = Settings::integer("url_lifetime");
	}
	catch(ProgrammingException& e)
	{
		current_url_lifetime_ = DEFAULT_URL_LIFETIME;
		Log::warn(e.message() + " Using the default value for setting the URL lifetime: " + QString::number(current_url_lifetime_));
	}
}

UrlManager& UrlManager::instance()
{
	static UrlManager url_manager;
	return url_manager;
}

void UrlManager::addNewUrl(UrlEntity in)
{
    instance().url_storage_.insert(in.string_id, in);
}

void UrlManager::removeUrl(QString id)
{
    if (instance().url_storage_.contains(id))
    {
        instance().url_storage_.remove(id);
    }
}

bool UrlManager::isInStorageAlready(QString filename_with_path)
{
    QList<QString> keys = instance().url_storage_.keys();
    for (int i = 0; i < keys.count(); i++)
    {
        if (instance().url_storage_.value(keys[i]).filename_with_path == filename_with_path)
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
        return instance().url_storage_.value(id);
    }
    return UrlEntity{};
}

QList<UrlEntity> UrlManager::getAllUrls()
{
    QList<UrlEntity> all_urls;
    QList<QString> keys = instance().url_storage_.keys();
    for (int i = 0; i < keys.count(); i++)
    {
        all_urls.append(instance().url_storage_.value(keys[i]));
    }
    return all_urls;
}

bool UrlManager::isValidUrl(QString token)
{
	UrlEntity cur_url = instance().getURLById(token);
	if (cur_url.isEmpty())
	{
		return false;
	}

	if (cur_url.created.addSecs(instance().current_url_lifetime_).toSecsSinceEpoch() <= QDateTime::currentDateTime().toSecsSinceEpoch())
	{
		return false;
	}
	return true;
}

void UrlManager::removeExpiredUrls()
{
	Log::info("Starting to cleanup URLs");
	QList<QString> to_be_removed {};

	QList<QString> keys = instance().url_storage_.keys();
	for (int i = 0; i < keys.count(); i++)
	{
		if (instance().url_storage_.value(keys[i]).created.toSecsSinceEpoch() < (QDateTime::currentDateTime().toSecsSinceEpoch()-instance().current_url_lifetime_))
		{
			to_be_removed.append(keys[i]);
		}
	}
	for (int i = 0; i < to_be_removed.count(); ++i)
	{
		instance().url_storage_.remove(to_be_removed[i]);
	}

	Log::info("Number of removed URLs: " + QString::number(to_be_removed.length()));
}

bool UrlManager::extendActiveUrls(QString ps_folder, int user_id)
{
	QList<QString> keys = instance().url_storage_.keys();
	bool has_active_urls = false;
	UrlEntity active_url = instance().getURLById(ps_folder);
	if (active_url.isEmpty()) return false;
	NGSD db;
	QString active_ps_id = db.processedSampleId(active_url.filename_with_path);

	for (int i = 0; i < keys.count(); i++)
	{
		if (instance().url_storage_.value(keys[i]).string_id == ps_folder ||
			(db.processedSampleId(instance().url_storage_.value(keys[i]).filename_with_path)==active_ps_id && instance().url_storage_.value(keys[i]).user_id==user_id))
		{
			Log::error(active_ps_id);
			Log::error(instance().url_storage_.value(keys[i]).string_id + " >> " + instance().url_storage_.value(keys[i]).filename);
			has_active_urls = true;
			UrlEntity url_to_be_updated = instance().url_storage_.value(keys[i]);
			url_to_be_updated.created = QDateTime::currentDateTime();
			instance().url_storage_.updateValue(keys[i], url_to_be_updated);
		}
	}
	return has_active_urls;
}
