#ifndef TRACKMANAGER_H
#define TRACKMANAGER_H

#include "cppVISUAL_global.h"

#include <QHash>
#include <QUuid>
#include <QObject>


class TrackWidget;

// a map of track id to track widget pointer; used to extract the explicit track from id on drop
class CPPVISUALSHARED_EXPORT TrackManager : QObject
{
	Q_OBJECT

public:
	static void addTrackWidget(QUuid, TrackWidget*);
	static bool removeTrackWidget(QUuid);
	static bool hasTrackWidget(QUuid);
	static TrackWidget* getTrackWidget(QUuid);

private:
	explicit TrackManager(QObject* parent = nullptr)
		: QObject(parent) {}
	static TrackManager& instance()
	{
		static TrackManager instance;
		return instance;
	}
	using TrackWidgetMap = QHash<QUuid, TrackWidget*>;

	TrackWidgetMap track_widgets_;
};



#endif // TRACKMANAGER_H
