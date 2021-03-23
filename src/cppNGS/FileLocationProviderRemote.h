#ifndef FILELOCATIONPROVIDERREMOTE_H
#define FILELOCATIONPROVIDERREMOTE_H

#include "cppNGS_global.h"
#include "FileLocationProvider.h"
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>

class CPPNGSSHARED_EXPORT FileLocationProviderRemote
	: virtual public FileLocationProvider
{
public:
	FileLocationProviderRemote(const QString sample_id, const QString server_host, const int server_port);
	virtual ~FileLocationProviderRemote() {}

	bool isLocal() const override;

	FileLocation getAnalysisVcf() const override;
	FileLocation getAnalysisSvFile() const override;
	FileLocation getAnalysisCnvFile() const override;
	FileLocation getAnalysisMosaicCnvFile() const override;
	FileLocation getAnalysisUpdFile() const override;
	FileLocation getRepeatExpansionImage(QString locus) const override;

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

	FileLocation getSomaticCnvCoverageFile() const override;
	FileLocation getSomaticCnvCallFile() const override;
	FileLocation getSomaticLowCoverageFile() const override;
	FileLocation getSomaticMsiFile() const override;

private:
	FileLocationList requestFileInfoByType(PathType type) const;
	FileLocation mapJsonObjectToFileLocation(QJsonObject obj) const;
	FileLocationList mapJsonArrayToFileLocationList(QJsonArray array) const;

protected:
	QString sample_id_;
	QString server_host_;
	int server_port_;

	QString getAnalysisPath() const override;
	QString getProjectPath() const override;
};

#endif // FILELOCATIONPROVIDERSERVER_H

