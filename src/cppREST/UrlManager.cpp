#include "UrlManager.h"
#include "Settings.h"
#include "Exceptions.h"
#include "Log.h"

UrlManager::UrlManager()
    : url_storage_()
{
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

    int url_lifetime = 0;
    try
    {
        url_lifetime = Settings::integer("url_lifetime");
    }
    catch(ProgrammingException& e)
    {
        url_lifetime = DEFAULT_URL_LIFETIME;
        Log::warn(e.message() + " Using the default value: " + QString::number(url_lifetime));
    }

    if (cur_url.created.addSecs(url_lifetime).toSecsSinceEpoch() <= QDateTime::currentDateTime().toSecsSinceEpoch())
    {
        return false;
    }
    return true;
}

void UrlManager::removeExpiredUrls()
{
    int url_lifetime = 0;
    try
    {
        url_lifetime = Settings::integer("url_lifetime");
    }
    catch(ProgrammingException& e)
    {
        url_lifetime = DEFAULT_URL_LIFETIME;
        Log::warn(e.message() + " Using the default value: " + QString::number(url_lifetime));
    }

    Log::info("Starting to cleanup URLs");
    QList<QString> to_be_removed {};
    QList<UrlEntity> to_be_backedup {};

    QList<QString> keys = instance().url_storage_.keys();
    for (int i = 0; i < keys.count(); i++)
    {
        if (instance().url_storage_.value(keys[i]).created.toSecsSinceEpoch() < (QDateTime::currentDateTime().toSecsSinceEpoch()-url_lifetime))
        {
            to_be_removed.append(keys[i]);
        }
        else
        {
            to_be_backedup.append(instance().url_storage_.value(keys[i]));
        }
    }
    for (int i = 0; i < to_be_removed.count(); ++i)
    {
        instance().url_storage_.remove(to_be_removed[i]);
    }

    Log::info("Number of removed URLs: " + QString::number(to_be_removed.length()));
}
