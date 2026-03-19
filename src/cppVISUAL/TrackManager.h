#ifndef TRACKMANAGER_H
#define TRACKMANAGER_H

#include "Track.h"

#include <QHash>
#include <QUuid>
#include <QSharedPointer>
#include <QObject>


class TrackManager : QObject
{
	Q_OBJECT

public:
	static void addTrack(QSharedPointer<Track> track);
	static bool hasTrack(QUuid);
	static bool removeTrack(QUuid);

	static void addTrackWidget(QUuid, QWidget*);
	static bool hasTrackWidget(QUuid);
	static QWidget* getTrackWidget(QUuid);

private:
	static TrackManager& instance()
	{
		static TrackManager instance;
		return instance;
	}
	using TrackMap = QHash<QUuid, QSharedPointer<Track>>;
	using TrackWidgetMap = QHash<QUuid, QWidget*>;

	TrackMap tracks;
	TrackWidgetMap track_widgets_;
};



#endif // TRACKMANAGER_H
