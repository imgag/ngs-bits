#ifndef FILELOCATIONPROVIDERFILESYSTEM_H
#define FILELOCATIONPROVIDERFILESYSTEM_H

#include <stdio.h>
#include "FileLocationProvider.h"

class FileLocationProviderFileSystem : public FileLocationProvider
{
public:
	static void doSomething();
protected:
   FileLocationProviderFileSystem(int,int);
   virtual ~FileLocationProviderFileSystem() {};
};


#endif // FILELOCATIONPROVIDERFILESYSTEM_H
