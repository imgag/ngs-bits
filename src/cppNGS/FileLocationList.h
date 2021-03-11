#ifndef FILELOCATIONLIST_H
#define FILELOCATIONLIST_H

#include "cppNGS_global.h"
#include <QStringList>
#include <QList>
#include "FileLocation.h"

class CPPNGSSHARED_EXPORT FileLocationList : public QList<FileLocation> {

public:
	FileLocationList();
	~FileLocationList();
	QStringList asStringList();
};

#endif // FILELOCATIONLIST_H
