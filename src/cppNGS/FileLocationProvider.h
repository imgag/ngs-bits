#ifndef FILELOCATIONPROVIDER_H
#define FILELOCATIONPROVIDER_H
#include "Exceptions.h"

#include "cppNGS_global.h"
#include "VariantList.h"
#include "Helper.h"
#include "FileLocation.h"
#include "FileLocationList.h"

class CPPNGSSHARED_EXPORT FileLocationProvider
{
public:
	virtual ~FileLocationProvider(){}

	//Returns a map of sample identifier to filename
	virtual FileLocationList getBamFiles() = 0;
	virtual FileLocationList getSegFilesCnv() = 0;
	virtual FileLocationList getIgvFilesBaf() = 0;
	virtual FileLocationList getMantaEvidenceFiles() = 0;
	virtual QString getEvidenceFile(QString bam_file) = 0;

	virtual FileLocationList getAnalysisLogFiles() = 0;
	virtual FileLocationList getCircosPlotFiles() = 0;
	virtual FileLocationList getVcfGzFiles() = 0;

	virtual FileLocationList getExpansionhunterVcfFiles() = 0;

	virtual FileLocationList getPrsTsvFiles() = 0;
	virtual FileLocationList getClincnvTsvFiles() = 0;
	virtual FileLocationList getLowcovBedFiles() = 0;
	virtual FileLocationList getStatLowcovBedFiles() = 0;
	virtual FileLocationList getCnvsClincnvSegFiles() = 0;
	virtual FileLocationList getCnvsClincnvTsvFiles() = 0;
	virtual FileLocationList getCnvsSegFiles() = 0;
	virtual FileLocationList getCnvsTsvFiles() = 0;
	virtual FileLocationList getRohsTsvFiles() = 0;

	virtual QString getAnalysisPath() = 0;
	virtual QString getProjectPath() = 0;
	virtual QString getRohFileAbsolutePath() = 0;

	virtual QString processedSampleName() = 0;
private:
	VariantList variants;
	QString filename;
};

#endif // FILELOCATIONPROVIDER_H
