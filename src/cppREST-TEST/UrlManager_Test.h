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
		QString file = "file.txt";

		IS_FALSE(UrlManager::isInStorageAlready(file));

		UrlManager::addNewUrl(url_id, UrlEntity(QFileInfo(file).fileName(), QFileInfo(file).absolutePath(), file, url_id, QDateTime::currentDateTime()));
		IS_TRUE(UrlManager::isInStorageAlready(file));

		S_EQUAL(UrlManager::getURLById(url_id).filename_with_path, file);

		UrlManager::removeUrl(url_id);
		IS_FALSE(UrlManager::isInStorageAlready(file));
	}
};
