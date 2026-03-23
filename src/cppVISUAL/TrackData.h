#ifndef TRACKDATA_H
#define TRACKDATA_H

#include "BedFile.h"

#include <QString>
#include <QColor>
#include <QUuid>
#include <QSharedPointer>


/*
 * TODO:
 * This will later store dynamic properties of each track
 * such as color, height, font size
 * maybe an ID so that it can be hashed
 */

enum TrackType
{
	BED_TRACK
};

struct TrackData
{
	//TODO to TrackWidget
	QUuid id;
	QString filename;
	QString name;

	//TODO to BedTrack
	QColor color = QColor(0, 0, 178);

	TrackData(QString filename, QString name)
		:id(QUuid::createUuid()), filename(filename), name(name)
	{
	}

	virtual ~TrackData() = default;
};


struct BedFileTrackData : public TrackData
{
	BedFile bedfile;

	BedFileTrackData(QString filename, QString name, BedFile bedfile)
		:TrackData(filename, name), bedfile(bedfile)
	{
	}
};


using TrackList = QVector<QSharedPointer<TrackData>>;


#endif // TRACKDATA_H
