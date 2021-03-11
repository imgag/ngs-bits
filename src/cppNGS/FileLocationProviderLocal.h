#ifndef FILELOCATIONPROVIDERLOCAL_H
#define FILELOCATIONPROVIDERLOCAL_H

#include "cppNGS_global.h"
#include "FileLocationProvider.h"
#include <QDir>

class CPPNGSSHARED_EXPORT FileLocationProviderLocal : virtual public FileLocationProvider
{
public:
	FileLocationProviderLocal(QString gsvar_file, const SampleHeaderInfo header_info, const AnalysisType analysis_type);
	virtual ~FileLocationProviderLocal() {}

	FileLocationList getBamFiles() override;
	FileLocationList getSegFilesCnv() override;
	FileLocationList getIgvFilesBaf() override;
	FileLocationList getMantaEvidenceFiles() override;
	QString getEvidenceFile(QString bam_file) override;

	FileLocationList getAnalysisLogFiles() override;
	FileLocationList getCircosPlotFiles() override;
	FileLocationList getVcfGzFiles() override;
	FileLocationList getExpansionhunterVcfFiles() override;
	FileLocationList getPrsTsvFiles() override;
	FileLocationList getClincnvTsvFiles() override;
	FileLocationList getLowcovBedFiles() override;
	FileLocationList getStatLowcovBedFiles() override;
	FileLocationList getCnvsClincnvSegFiles() override;
	FileLocationList getCnvsClincnvTsvFiles() override;
	FileLocationList getCnvsSegFiles() override;
	FileLocationList getCnvsTsvFiles() override;
	FileLocationList getRohsTsvFiles() override;

	QString getAnalysisPath() override;
	QString getProjectPath() override;
	QString getRohFileAbsolutePath() override;

	QString processedSampleName() override;

private:
	FileLocationList mapFoundFilesToFileLocation(QStringList& files, PathType type);
	static void addToList(const FileLocation& loc, FileLocationList& list);

protected:
	QString gsvar_file_;
	SampleHeaderInfo header_info_;
	AnalysisType analysis_type_;
};

#endif // FILELOCATIONPROVIDERLOCAL_H
