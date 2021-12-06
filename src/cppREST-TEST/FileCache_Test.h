#include "TestFramework.h"
#include "FileCache.h"
#include "ServerHelper.h"

TEST_CLASS(FileCache_Test)
{
Q_OBJECT
private slots:

	void test_caching_meachanism()
	{
		QByteArray filename = TESTDATA("data_in/text.txt");
		QFile file(filename);
		file.open(QIODevice::ReadOnly);
		QByteArray content = file.readAll();
		qDebug() << content;
		QString generatedId = ServerHelper::generateUniqueStr();

		IS_FALSE(FileCache::isInCacheAlready(filename));

		FileCache::addFileToCache(generatedId, filename, content, file.size());

		QString foundId = FileCache::getFileIdIfInCache(filename);
		S_EQUAL(foundId, generatedId);

		IS_TRUE(FileCache::isInCacheAlready(filename));

		StaticFile fileFromCache = FileCache::getFileById(foundId);
		S_EQUAL(fileFromCache.content, content);

		FileCache::removeFileFromCache(foundId);
		IS_FALSE(FileCache::isInCacheAlready(filename));
	}
};
