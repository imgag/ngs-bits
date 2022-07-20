#ifndef FILELOCATIONPROVIDERLOCAL_H
#define FILELOCATIONPROVIDERLOCAL_H

#include "cppNGSD_global.h"
#include "FileLocationProvider.h"
#include "KeyValuePair.h"

class CPPNGSDSHARED_EXPORT FileLocationProviderLocal
	: virtual public FileLocationProvider
{
public:
	FileLocationProviderLocal(QString gsvar_file, const SampleHeaderInfo& header_info, AnalysisType analysis_type);
	virtual ~FileLocationProviderLocal() {}

	bool isLocal() const override;

	FileLocation getAnalysisVcf() const override;
	FileLocation getAnalysisMosaicFile() const override;
	FileLocation getAnalysisSvFile() const override;
	FileLocation getAnalysisCnvFile() const override;
	FileLocation getAnalysisMosaicCnvFile() const override;
	FileLocation getAnalysisUpdFile() const override;
	FileLocation getRepeatExpansionImage(QString locus) const override;
	FileLocationList getQcFiles() const override;

	FileLocationList getVcfFiles(bool return_if_missing) const override;
	FileLocationList getBamFiles(bool return_if_missing) const override;
	FileLocationList getCnvCoverageFiles(bool return_if_missing) const override;
	FileLocationList getBafFiles(bool return_if_missing) const override;
	FileLocationList getMantaEvidenceFiles(bool return_if_missing) const override;
	FileLocationList getCircosPlotFiles(bool return_if_missing) const override;
	FileLocationList getRepeatExpansionFiles(bool return_if_missing) const override;
	FileLocationList getPrsFiles(bool return_if_missing) const override;
	FileLocationList getLowCoverageFiles(bool return_if_missing) const override;
	FileLocationList getCopyNumberCallFiles(bool return_if_missing) const override;
	FileLocationList getRohFiles(bool return_if_missing) const override;
	FileLocationList getExpressionFiles(bool return_if_missing) const override;
	FileLocationList getExonExpressionFiles(bool return_if_missing) const override;
	FileLocationList getSomaticLowCoverageFiles(bool return_if_missing) const override;

	FileLocation getSomaticCnvCoverageFile() const override;
	FileLocation getSomaticCnvCallFile() const override;
	FileLocation getSomaticLowCoverageFile() const override;
	FileLocation getSomaticMsiFile() const override;
	FileLocation getSomaticIgvScreenshotFile() const override;
	FileLocation getSomaticCfdnaCandidateFile() const override;

private:
	static void addToList(const FileLocation& loc, FileLocationList& list, bool add_if_missing=true); //Make add_if_missing mandatory when all

	//Returns base location for sample-specific files, i.e. the GSvar file name without extension. From the base locations other file names can be generated.
	QList<KeyValuePair> getBaseLocations() const;

	//Returns analysis path, i.e. the path of the GSvar file
	QString getAnalysisPath() const;
	//Returns the project path , i.e. the parent directory of the analysis path
	QString getProjectPath() const;

protected:
	QString gsvar_file_;
	SampleHeaderInfo header_info_;
	AnalysisType analysis_type_;
};

#endif // FILELOCATIONPROVIDERLOCAL_H
