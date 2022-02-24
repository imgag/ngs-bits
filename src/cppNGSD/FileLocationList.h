#ifndef FILELOCATIONLIST_H
#define FILELOCATIONLIST_H

#include "cppNGSD_global.h"
#include <QStringList>
#include "FileLocation.h"

class CPPNGSDSHARED_EXPORT FileLocationList
	: public QList<FileLocation>
{
public:
	FileLocationList();

	//Filters the list by ID, i.e. sample name
	FileLocationList filterById(const QString& id) const;

	QStringList asStringList() const;
};

#endif // FILELOCATIONLIST_H
