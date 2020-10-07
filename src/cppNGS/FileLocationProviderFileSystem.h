#ifndef FILELOCATIONPROVIDERFILESYSTEM_H
#define FILELOCATIONPROVIDERFILESYSTEM_H

#include "cppNGS_global.h"
#include "FileLocationProvider.h"

class CPPNGSSHARED_EXPORT FileLocationProviderFileSystem : virtual public FileLocationProvider
{
public:
	FileLocationProviderFileSystem(QString gsvar_file, const SampleHeaderInfo header_info, const AnalysisType analysis_type);
	virtual ~FileLocationProviderFileSystem() {}

	QList<FileLocation> getBamFiles() override;
	QList<FileLocation> getSegFilesCnv() override;
protected:
	QString gsvar_file_;
	SampleHeaderInfo header_info_;
	AnalysisType analysis_type_;
};


#endif // FILELOCATIONPROVIDERFILESYSTEM_H
