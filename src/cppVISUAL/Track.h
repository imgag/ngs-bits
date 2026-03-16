#ifndef TRACK_H
#define TRACK_H

#include "BedFile.h"
#include <QString>
#include <QColor>


/*
 * TODO:
 * This will later store dynamic properties of each track
 * such as color, height, font size
 * maybe an ID so that it can be hashed
 */
struct Track
{
	/*must provide*/
	QString filename;
	QString name;
	BedFile bedfile;

	/*optional*/
	QColor color = QColor(0, 0, 125);
};


#endif // TRACK_H
