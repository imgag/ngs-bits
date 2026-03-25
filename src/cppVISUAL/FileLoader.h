#ifndef FILELOADER_H
#define FILELOADER_H

#include "cppVISUAL_global.h"
#include "TrackWidget.h"

#include <QSharedPointer>
#include <QFileInfo>

class CPPVISUALSHARED_EXPORT FileLoader
{
public:
	// factory function that loads depending on the extension of the file
	static TrackWidgetList load(QString file_path, QWidget* parent = nullptr);
	// loads Bed File - emits just one TrackWidget
	static TrackWidgetList loadBedFile(QString file_path, QWidget* parent = nullptr);
};

#endif // FILELOADER_H
