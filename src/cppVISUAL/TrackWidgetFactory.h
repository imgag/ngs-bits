#ifndef TRACKWIDGETFACTORY_H
#define TRACKWIDGETFACTORY_H

#include "BedTrack.h"
#include "TrackData.h"
#include "TrackWidget.h"

class TrackWidgetFactory
{
public:
	static TrackWidget* getTrackWidget(QWidget* parent, QSharedPointer<TrackData> track)
	{
		if (auto bed_track = qSharedPointerDynamicCast<BedFileTrackData>(track)) return new BedTrack(parent, bed_track);
		qDebug() << "Unable to convert track into available classes" << Qt::endl;
		return nullptr;
	}
};


#endif // TRACKWIDGETFACTORY_H
