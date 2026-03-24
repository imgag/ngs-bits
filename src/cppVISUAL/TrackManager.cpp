#include "TrackManager.h"

void TrackManager::addTrackWidget(QUuid id, TrackWidget* widget)
{
	instance().track_widgets_[id] = widget;
}

TrackWidget* TrackManager::getTrackWidget(QUuid id)
{
	return instance().track_widgets_[id];
}

bool TrackManager::hasTrackWidget(QUuid id)
{
	return instance().track_widgets_.contains(id);
}

bool TrackManager::removeTrackWidget(QUuid id)
{
	return instance().track_widgets_.remove(id);
}
