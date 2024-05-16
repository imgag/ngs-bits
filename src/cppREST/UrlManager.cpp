#include "UrlManager.h"
#include "Helper.h"

UrlManager::UrlManager()
{
}

UrlManager& UrlManager::instance()
{
	static UrlManager url_manager;
	return url_manager;
}

void UrlManager::addNewUrl(UrlEntity in)
{
    FileDbManager::addUrl(in);
}

void UrlManager::removeUrl(const QString& id)
{
    FileDbManager::removeUrl(id);
}

bool UrlManager::isInStorageAlready(const QString& filename_with_path)
{
    return FileDbManager::isFileInStoreAlready(filename_with_path);
}

UrlEntity UrlManager::getURLById(const QString& id)
{
    return FileDbManager::getUrl(id);
}

bool UrlManager::isValidUrl(QString token)
{
    UrlEntity cur_url = FileDbManager::getUrl(token);
    if (cur_url.isEmpty())
    {
        return false;
    }

    int url_lifetime = ServerHelper::getNumSettingsValue("url_lifetime"); // URL lifetime in seconds
    if (url_lifetime == 0) url_lifetime = DEFAULT_URL_LIFETIME; // default value, if not set in the config
    if ((QDateTime::currentDateTime().toSecsSinceEpoch() - cur_url.created.toSecsSinceEpoch()) >= url_lifetime)
    {
        return false;
    }
    return true;
}

int UrlManager::removeExpiredUrls()
{
    int url_lifetime = ServerHelper::getNumSettingsValue("url_lifetime"); // URL lifetime in seconds
    if (url_lifetime == 0) url_lifetime = DEFAULT_URL_LIFETIME; // default value, if not set in the config

    int removed_count = 0;
    Log::info("Starting to cleanup URLs");
    QList<UrlEntity> all_urls = FileDbManager::getAllUrls();
    for (int i = 0; i < all_urls.size(); i++)
    {
        if ((QDateTime::currentDateTime().toSecsSinceEpoch() - all_urls[i].created.toSecsSinceEpoch()) < url_lifetime)
        {
            continue;
        }
        FileDbManager::removeUrl(all_urls[i].string_id);
        removed_count++;
    }

    Log::info("Number of removed URLs: " + QString::number(removed_count));
    return removed_count;
}
