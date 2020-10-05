#ifndef FILELOCATIONPROVIDER_H
#define FILELOCATIONPROVIDER_H
#include "Exceptions.h"

#include "cppNGS_global.h"
#include "VariantList.h"
#include "qfileinfo.h"
#include "FileLocationHelper.h"

struct FileLocation
{
	QString id; //sample identifier/name
	PathType type; //file type
	QString filename; //file name
};

class CPPNGSSHARED_EXPORT FileLocationProvider
{
public:
	virtual ~FileLocationProvider(){}

	//Returns a map of sample identifier to filename
	virtual QList<FileLocation> getBamFiles() = 0;

private:
	VariantList variants;
	QString filename;
};




#endif // FILELOCATIONPROVIDER_H
