#include "BamTrackDataManager.h"
#include "FileLoader.h"

#include "QFileInfo"

static QHash<GenomeVisualizationWidget*, QHash<QString, QWeakPointer<BamTrackData>>> caches_;

QSharedPointer<BamTrackData> BamTrackDataManager::getOrCreate(GenomeVisualizationWidget* viewer, QString file_path)
{
	if (!caches_.contains(viewer))
	{
		QObject::connect(viewer, &QObject::destroyed, [viewer]() { caches_.remove(viewer); });
	}
	auto& cache_ = caches_[viewer];
	QString abs_name = Helper::isHttpUrl(file_path) ? file_path : QFileInfo(file_path).absoluteFilePath();

	if (cache_.contains(abs_name))
	{
		auto existing = cache_[abs_name].lock();
		if (existing) return existing;
	}

	QSharedPointer<BamReader> reader = FileLoader::loadBamFile(file_path);
	if (!reader) return nullptr;

	auto track_data = QSharedPointer<BamTrackData>::create(viewer, file_path);
	track_data->setBamReader(reader);
	cache_[abs_name] = track_data.toWeakRef();
	return track_data;
}

void BamTrackDataManager::reload(GenomeVisualizationWidget* viewer, QString file_path)
{
	if (!caches_.contains(viewer)) return;
	auto& cache_ = caches_[viewer];
	QString abs_name = Helper::isHttpUrl(file_path) ? file_path : QFileInfo(file_path).absoluteFilePath();

	if (cache_.contains(abs_name))
	{
		auto existing = cache_[abs_name].lock();
		if (existing) existing->reload();
	}
}

