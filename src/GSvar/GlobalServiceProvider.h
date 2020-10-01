#ifndef GLOBALSERVICEPROVIDER_H
#define GLOBALSERVICEPROVIDER_H

//#include "FileLocationProvider.h"
#include "FileLocationProviderFileSystem.h"
#include "FileLocationProviderNGSD.h"

class GlobalServiceProvider : public FileLocationProviderFileSystem, public FileLocationProviderNGSD
{
public:
	static GlobalServiceProvider* getInstance();
//	static bool exists();

	int getVariants();
	void setVariants(int _in);

	int getFilename();
	void setFilename(int _in);

//	virtual int getBamFilesInFileSystem(int p);
//	virtual int getBamFilesInNGSD(int p);

//	inline int getVariants(){ return variants; }
//	inline void setVariants(int _in){ variants = _in; }

//	inline int getFilename(){ return filename; }
//	inline void setFilename(int _in){ filename = _in; }
	~GlobalServiceProvider(){}
protected:
//	virtual ~GlobalServiceProvider(){}
	static GlobalServiceProvider* theOnlyInstance;
private:
	GlobalServiceProvider();
	int variants;
	int filename;
};


#endif // GLOBALSERVICEPROVIDER_H
