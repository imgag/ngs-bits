#ifndef FILELOCATIONPROVIDERNGSD_H
#define FILELOCATIONPROVIDERNGSD_H


#include "cppNGSD_global.h"
#include "FileLocationProvider.h"



class CPPNGSDSHARED_EXPORT FileLocationProviderNGSD : virtual public FileLocationProvider
{
public:
	int getBamFilesInNGSD(int p);
protected:
	virtual ~FileLocationProviderNGSD() {}
};


#endif // FILELOCATIONPROVIDERNGSD_H
