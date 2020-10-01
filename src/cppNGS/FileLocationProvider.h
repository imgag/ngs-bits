#ifndef FILELOCATIONPROVIDER_H
#define FILELOCATIONPROVIDER_H


#include "cppNGS_global.h"

class CPPNGSSHARED_EXPORT FileLocationProvider
{
public:
	virtual ~FileLocationProvider(){}

	virtual int getVariants() = 0;
	virtual void setVariants(int _in) = 0;

	virtual int getFilename() = 0;
	virtual void setFilename(int _in) = 0;

private:
	int variants;
	int filename;
};




#endif // FILELOCATIONPROVIDER_H
