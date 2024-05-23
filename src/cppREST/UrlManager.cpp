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
    if (cur_url.created.addSecs(url_lifetime).toSecsSinceEpoch() <= QDateTime::currentDateTime().toSecsSinceEpoch())
    {
        return false;
    }
    return true;
}

void UrlManager::removeExpiredUrls()
{
    int url_lifetime = ServerHelper::getNumSettingsValue("url_lifetime"); // URL lifetime in seconds
    if (url_lifetime == 0) url_lifetime = DEFAULT_URL_LIFETIME; // default value, if not set in the config

    Log::info("Starting to cleanup URLs");
    int current_count = FileDbManager::getUrlsCount();
    Log::info("Number of active URLs: " + QString::number(current_count));
    FileDbManager::removeUrlsOlderThan(QDateTime::currentDateTime().toSecsSinceEpoch()-url_lifetime);

    int new_count = FileDbManager::getUrlsCount();
    Log::info("Number of active URLs after the cleanup: " + QString::number(new_count));
}
