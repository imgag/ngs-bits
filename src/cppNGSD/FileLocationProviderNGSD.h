#ifndef FILELOCATIONPROVIDERNGSD_H
#define FILELOCATIONPROVIDERNGSD_H

#include <stdio.h>
#include "FileLocationProvider.h"

class CPPNGSSHARED_EXPORT FileLocationProviderNGSD : public FileLocationProvider
{
public:
	static void doSomethingElse();
protected:
   FileLocationProviderNGSD(int,int);
   virtual ~FileLocationProviderNGSD() {};
};


#endif // FILELOCATIONPROVIDERNGSD_H
