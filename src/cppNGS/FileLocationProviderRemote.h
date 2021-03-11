#ifndef FILELOCATIONPROVIDERREMOTE_H
#define FILELOCATIONPROVIDERREMOTE_H

#include "cppNGS_global.h"
#include "FileLocationProvider.h"
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>

class CPPNGSSHARED_EXPORT FileLocationProviderRemote : virtual public FileLocationProvider
{
public:
	FileLocationProviderRemote(const QString sample_id, const QString server_host, const int server_port);
	virtual ~FileLocationProviderRemote() {}

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
	void setIsFoundFlag(FileLocation& file);
	FileLocationList requestFileInfoByType(PathType type);
	FileLocation mapJsonObjectToFileLocation(QJsonObject obj);
	FileLocationList mapJsonArrayToFileLocationList(QJsonArray array);

protected:
	QString sample_id_;
	QString server_host_;
	int server_port_;
};

#endif // FILELOCATIONPROVIDERSERVER_H
