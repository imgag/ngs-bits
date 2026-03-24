#ifndef TRACKMANAGER_H
#define TRACKMANAGER_H

#include "cppVISUAL_global.h"

#include <QHash>
#include <QUuid>
#include <QObject>


class TrackWidget;

class CPPVISUALSHARED_EXPORT TrackManager : QObject
{
	Q_OBJECT

public:
	static void addTrackWidget(QUuid, TrackWidget*);
	static bool removeTrackWidget(QUuid);
	static bool hasTrackWidget(QUuid);
	static TrackWidget* getTrackWidget(QUuid);

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
