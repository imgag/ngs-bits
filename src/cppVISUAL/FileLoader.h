#ifndef FILELOADER_H
#define FILELOADER_H

#include "cppVISUAL_global.h"
#include "TrackWidget.h"
#include "BedFile.h"

#include <QSharedPointer>
#include <QFileInfo>

class CPPVISUALSHARED_EXPORT FileLoader
{
public:
	// factory function that loads depending on the extension of the file
	static TrackWidgetList loadTracks(QString file_path, QWidget* parent = nullptr);
	// loads Bed File - emits just one TrackWidget
	static TrackWidgetList loadBedFileTracks(QString file_path, QWidget* parent = nullptr);
	// loads bed file
	static QSharedPointer<BedFile> loadBedFile(QString file_path);
};

#endif // FILELOADER_H
