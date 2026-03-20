#ifndef TRACKMANAGER_H
#define TRACKMANAGER_H

#include "TrackData.h"

#include <QHash>
#include <QUuid>
#include <QSharedPointer>
#include <QObject>


class TrackManager : QObject
{
	Q_OBJECT

public:
	static void addTrackWidget(QUuid, QWidget*);
	static void addTrackWidget(QSharedPointer<TrackData>, QWidget*);
	static bool removeTrackWidget(QUuid);
	static bool removeTrackWidget(QSharedPointer<TrackData>);
	static bool hasTrackWidget(QUuid);
	static bool hasTrackWidget(QSharedPointer<TrackData>);
	static QWidget* getTrackWidget(QUuid);
	static QWidget* getTrackWidget(QSharedPointer<TrackData>);

private:
	static TrackManager& instance()
	{
		static TrackManager instance;
		return instance;
	}
	using TrackWidgetMap = QHash<QUuid, QWidget*>;

	TrackWidgetMap track_widgets_;
};



#endif // TRACKMANAGER_H
