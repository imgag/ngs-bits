#ifndef TRACKMANAGER_H
#define TRACKMANAGER_H

#include "TrackData.h"

#include <QHash>
#include <QUuid>
#include <QSharedPointer>
#include <QObject>


class TrackWidget;

class TrackManager : QObject
{
	Q_OBJECT

public:
	static void addTrackWidget(QUuid, TrackWidget*);
	static void addTrackWidget(QSharedPointer<TrackData>, TrackWidget*);
	static bool removeTrackWidget(QUuid);
	static bool removeTrackWidget(QSharedPointer<TrackData>);
	static bool hasTrackWidget(QUuid);
	static bool hasTrackWidget(QSharedPointer<TrackData>);
	static TrackWidget* getTrackWidget(QUuid);
	static TrackWidget* getTrackWidget(QSharedPointer<TrackData>);

private:
	static TrackManager& instance()
	{
		static TrackManager instance;
		return instance;
	}
	using TrackWidgetMap = QHash<QUuid, TrackWidget*>;

	TrackWidgetMap track_widgets_;
};



#endif // TRACKMANAGER_H
