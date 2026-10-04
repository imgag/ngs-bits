#include "BamTrackDataManager.h"
#include "FileLoader.h"

#include "QFileInfo"

/*
 * TODO: even though this uses a weak ptr, it's better to switch to LRUCache
 */
static QHash<QString, QWeakPointer<BamTrackData>> cache_;

QSharedPointer<BamTrackData> BamTrackDataManager::getOrCreate(QString file_path)
{
	QFileInfo file_info(file_path);
	QString abs_name = file_info.absoluteFilePath();

	if (cache_.contains(abs_name))
	{
		auto existing = cache_[abs_name].lock();
		if (existing) return existing;
	}

	QSharedPointer<BamReader> reader = FileLoader::loadBamFile(file_path);
	if (!reader) return nullptr;

	auto track_data = QSharedPointer<BamTrackData>::create(file_path);
	track_data->setBamReader(reader);
	cache_[file_path] = track_data.toWeakRef();
	return track_data;
}

void BamTrackDataManager::reload(QString file_path)
{
	QFileInfo file_info(file_path);
	QString abs_name = file_info.absoluteFilePath();

	if (cache_.contains(abs_name))
	{
		auto existing = cache_[abs_name].lock();
		if (existing) existing->reload();
	}
}


