#ifndef FILELOCATIONPROVIDER_H
#define FILELOCATIONPROVIDER_H
#include "Exceptions.h"

#include "cppNGS_global.h"
#include "VariantList.h"
#include "qfileinfo.h"

struct IgvFile
{
	QString id; //sample identifier/name (for visualization)
	QString type; //file type (for grouping)
	QString filename; //file name
};

class CPPNGSSHARED_EXPORT FileLocationProvider
{
public:
	virtual ~FileLocationProvider(){}

	virtual VariantList getVariants() = 0;
	virtual void setVariants(VariantList v) = 0;

	virtual QString getFilename() = 0;
	virtual void setFilename(QString f) = 0;

private:
	VariantList variants;
	QString filename;
};




#endif // FILELOCATIONPROVIDER_H
