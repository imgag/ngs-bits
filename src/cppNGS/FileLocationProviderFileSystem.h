#ifndef FILELOCATIONPROVIDERFILESYSTEM_H
#define FILELOCATIONPROVIDERFILESYSTEM_H

#include "cppNGS_global.h"
#include "FileLocationProvider.h"

class CPPNGSSHARED_EXPORT FileLocationProviderFileSystem : virtual public FileLocationProvider
{
public:
	FileLocationProviderFileSystem(QString gsvar_file, const SampleHeaderInfo header_info);
	virtual ~FileLocationProviderFileSystem() {}

	QList<FileLocation> getBamFiles() override;
protected:
	QString gsvar_file_;
	SampleHeaderInfo header_info_;
};


#endif // FILELOCATIONPROVIDERFILESYSTEM_H
