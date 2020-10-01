#ifndef GLOBALSERVICEPROVIDER_H
#define GLOBALSERVICEPROVIDER_H

#include "FileLocationProviderFileSystem.h"
#include "FileLocationProviderNGSD.h"

class GlobalServiceProvider : public FileLocationProviderFileSystem, public FileLocationProviderNGSD
{
public:
	static GlobalServiceProvider* getInstance();
	static bool exists();

	int getVariants();
	void setVariants(int _in);

	int getFilename();
	void setFilename(int _in);


protected:
	static GlobalServiceProvider* theOnlyInstance;
private:
	GlobalServiceProvider();
	~GlobalServiceProvider(){}
	int variants;
	int filename;
};

#endif // GLOBALSERVICEPROVIDER_H
