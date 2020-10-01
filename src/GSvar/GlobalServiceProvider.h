#ifndef GLOBALSERVICEPROVIDER_H
#define GLOBALSERVICEPROVIDER_H

#include "FileLocationProviderFileSystem.h"
#include "FileLocationProviderNGSD.h"


class GlobalServiceProvider : public FileLocationProviderFileSystem, public FileLocationProviderNGSD
{
public:
	static GlobalServiceProvider* getInstance();
	static bool exists();

	VariantList getVariants();
	void setVariants(VariantList v);

	QString getFilename();
	void setFilename(QString f);


protected:
	static GlobalServiceProvider* theOnlyInstance;
private:
	GlobalServiceProvider();
	~GlobalServiceProvider(){}
	VariantList variants;
	QString filename;
};

#endif // GLOBALSERVICEPROVIDER_H
