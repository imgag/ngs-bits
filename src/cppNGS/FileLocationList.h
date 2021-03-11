#ifndef FILELOCATIONLIST_H
#define FILELOCATIONLIST_H

#include "cppNGS_global.h"
#include <QStringList>
#include "FileLocation.h"

class CPPNGSSHARED_EXPORT FileLocationList
	: public QList<FileLocation>
{
public:
	FileLocationList();
	~FileLocationList();
	QStringList asStringList() const;
};

#endif // FILELOCATIONLIST_H
