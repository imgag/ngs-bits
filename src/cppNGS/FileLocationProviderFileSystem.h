#ifndef FILELOCATIONPROVIDERFILESYSTEM_H
#define FILELOCATIONPROVIDERFILESYSTEM_H

#include "cppNGS_global.h"
#include "FileLocationProvider.h"

class CPPNGSSHARED_EXPORT FileLocationProviderFileSystem : virtual public FileLocationProvider
{
public:
	int getBamFilesInFileSystem();
protected:
	virtual ~FileLocationProviderFileSystem() {}
};


#endif // FILELOCATIONPROVIDERFILESYSTEM_H
