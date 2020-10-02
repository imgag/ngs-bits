#ifndef FILELOCATIONPROVIDERNGSD_H
#define FILELOCATIONPROVIDERNGSD_H


#include "cppNGSD_global.h"
#include "FileLocationProvider.h"

class CPPNGSDSHARED_EXPORT FileLocationProviderNGSD
	: public FileLocationProvider
{
public:
	FileLocationProviderNGSD(QString procssed_sample_id);

	virtual ~FileLocationProviderNGSD() {}

	QList<FileLocation> getBamFiles() override;

protected:
	QString procssed_sample_id_;
};


#endif // FILELOCATIONPROVIDERNGSD_H
