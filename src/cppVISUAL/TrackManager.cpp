#include "TrackManager.h"


void TrackManager::addTrack(QSharedPointer<Track> track)
{
	instance().tracks[track->id] = track;
}

bool TrackManager::hasTrack(QUuid id)
{
	return instance().tracks.contains(id);
}

bool TrackManager::removeTrack(QUuid id)
{
	return instance().tracks.remove(id);
}

void TrackManager::addTrackWidget(QUuid id, QWidget* widget)
{
	instance().track_widgets_[id] = widget;
}

QWidget* TrackManager::getTrackWidget(QUuid id)
{
	return instance().track_widgets_[id];
}

bool TrackManager::hasTrackWidget(QUuid id)
{
	return instance().track_widgets_.contains(id);
}
