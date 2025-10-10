#include "TestFramework.h"
#include "FileMetaCache.h"
#include "FastFileInfo.h"

TEST_CLASS(FileMetaCache_Test)
{
private:

    TEST_METHOD(test_file_metadata_caching)
    {
        QString file = TESTDATA("data_in/picture.png");

        IS_FALSE(FileMetaCache::isInStorageAlready(file));
        FastFileInfo *info = new FastFileInfo(file);
        S_EQUAL(info->absoluteFilePath(), file);

        FileMetaCache::addMetadata(FileMetadata(info->absoluteFilePath(), info->absolutePath(), info->fileName(), true, info->size(), true, info->exists(), info->lastModified(), QDateTime::currentDateTime()));
        FileMetadata metadata = FileMetaCache::getMetadata(info->absoluteFilePath());

        I_EQUAL(info->size(), metadata.size);
        IS_TRUE(metadata.has_existence_info);
        IS_TRUE(FileMetaCache::isInStorageAlready(info->absoluteFilePath()));

        FileMetaCache::removeMetadata(metadata.absolute_file_path);

        IS_FALSE(FileMetaCache::isInStorageAlready(info->absoluteFilePath()));
    }
};
