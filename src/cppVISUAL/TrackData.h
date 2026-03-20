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

struct TrackData
{
	QUuid id;

	/*must provide*/
	QString filename;
	QString name;
	BedFile bedfile;

	/*optional*/
	QColor color = QColor(0, 0, 178);

	TrackData(QString filename, QString name, BedFile bedfile)
		:id(QUuid::createUuid()), filename(filename), name(name),
		bedfile(bedfile)
	{
	}
};

using TrackList = QVector<QSharedPointer<TrackData>>;


#endif // TRACKDATA_H
