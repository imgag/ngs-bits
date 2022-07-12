#ifndef FILELOCATIONPROVIDERREMOTE_H
#define FILELOCATIONPROVIDERREMOTE_H

#include "cppNGSD_global.h"
#include "FileLocationProvider.h"
#include "LoginManager.h"
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>

class CPPNGSDSHARED_EXPORT FileLocationProviderRemote
	: virtual public FileLocationProvider
{
public:
	FileLocationProviderRemote(const QString sample_id);
	virtual ~FileLocationProviderRemote() {}

	bool isLocal() const override;

	FileLocation getAnalysisVcf() const override;
	FileLocation getAnalysisMosaicFile() const override;
	FileLocation getAnalysisSvFile() const override;
	FileLocation getAnalysisCnvFile() const override;
	FileLocation getAnalysisMosaicCnvFile() const override;
	FileLocation getAnalysisUpdFile() const override;
	FileLocation getRepeatExpansionImage(QString locus) const override;
	FileLocationList getQcFiles() const override;

	FileLocationList getBamFiles(bool return_if_missing) const override;
	FileLocationList getCnvCoverageFiles(bool return_if_missing) const override;
	FileLocationList getBafFiles(bool return_if_missing) const override;
	FileLocationList getMantaEvidenceFiles(bool return_if_missing) const override;

	FileLocationList getCircosPlotFiles(bool return_if_missing) const override;
	FileLocationList getVcfFiles(bool return_if_missing) const override;
	FileLocationList getRepeatExpansionFiles(bool return_if_missing) const override;
	FileLocationList getPrsFiles(bool return_if_missing) const override;
	FileLocationList getLowCoverageFiles(bool return_if_missing) const override;
	FileLocationList getCopyNumberCallFiles(bool return_if_missing) const override;
	FileLocationList getRohFiles(bool return_if_missing) const override;
	FileLocationList getExpressionFiles(bool return_if_missing) const override;
	FileLocationList getSomaticLowCoverageFiles(bool return_if_missing) const override;

	FileLocation getSomaticCnvCoverageFile() const override;
	FileLocation getSomaticCnvCallFile() const override;
	FileLocation getSomaticLowCoverageFile() const override;
	FileLocation getSomaticMsiFile() const override;
	FileLocation getSomaticIgvScreenshotFile() const override;
	FileLocation getSomaticCfdnaCandidateFile() const override;

private:
	FileLocationList getFileLocationsByType(PathType type, bool return_if_missing) const;
	FileLocation getOneFileLocationByType(PathType type, QString locus) const;
	FileLocation mapJsonObjectToFileLocation(QJsonObject obj) const;
	FileLocationList mapJsonArrayToFileLocationList(QJsonArray array, bool return_if_missing) const;

protected:
	QString sample_id_;
};

#endif // FILELOCATIONPROVIDERSERVER_H

