#include "TestFramework.h"
#include "UrlManager.h"
#include "ServerHelper.h"

TEST_CLASS(UrlManager_Test)
{
Q_OBJECT
private slots:

	void test_url_manager()
	{
		QString url_id = ServerHelper::generateUniqueStr();
		QString filename = "file.txt";

		IS_FALSE(UrlManager::isInStorageAlready(filename));

		UrlManager::addUrlToStorage(url_id, filename);
		IS_TRUE(UrlManager::isInStorageAlready(filename));

		S_EQUAL(UrlManager::getURLById(url_id).filename_with_path, filename);

		UrlManager::removeUrlFromStorage(url_id);
		IS_FALSE(UrlManager::isInStorageAlready(filename));
	}
};
