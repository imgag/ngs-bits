#include "TrackManager.h"

void TrackManager::addTrackWidget(QUuid id, TrackWidget* widget)
{
	instance().track_widgets_[id] = widget;
}

void TrackManager::addTrackWidget(QSharedPointer<TrackData> track, TrackWidget* widget)
{
	if (track) addTrackWidget(track->id, widget);
}

TrackWidget* TrackManager::getTrackWidget(QUuid id)
{
	return instance().track_widgets_[id];
}


TrackWidget* TrackManager::getTrackWidget(QSharedPointer<TrackData> track)
{
	if (track) return getTrackWidget(track->id);
	return nullptr;
}

bool TrackManager::hasTrackWidget(QUuid id)
{
	return instance().track_widgets_.contains(id);
}

bool TrackManager::hasTrackWidget(QSharedPointer<TrackData> track)
{
	if (track) return hasTrackWidget(track->id);
	return false;
}

bool TrackManager::removeTrackWidget(QUuid id)
{
	return instance().track_widgets_.remove(id);
}

bool TrackManager::removeTrackWidget(QSharedPointer<TrackData> track)
{
	if (track) removeTrackWidget(track->id);
	return false;
}
