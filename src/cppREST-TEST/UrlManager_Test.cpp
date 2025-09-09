#include "TestFramework.h"
#include "UrlManager.h"
#include "ServerHelper.h"

TEST_CLASS(UrlManager_Test)
{
private:

	TEST_METHOD(test_url_manager)
	{        
        QString url_id = ServerHelper::generateUniqueStr();
        QString file = "file.txt";

        IS_FALSE(UrlManager::isInStorageAlready(file));
        QFileInfo info = QFileInfo(file);
        UrlEntity cur_url = UrlEntity(url_id, info.fileName(), info.absolutePath(), file, url_id, info.size(), info.exists(), QDateTime::currentDateTime());
        UrlManager::addNewUrl(cur_url);
		IS_TRUE(UrlManager::isInStorageAlready(file));

		S_EQUAL(UrlManager::getURLById(url_id).filename_with_path, file);

		UrlManager::removeUrl(url_id);
		IS_FALSE(UrlManager::isInStorageAlready(file));
	}
};
