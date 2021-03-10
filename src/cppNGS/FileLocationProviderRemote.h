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

	QList<FileLocation> getBamFiles() override;
	QList<FileLocation> getSegFilesCnv() override;
	QList<FileLocation> getIgvFilesBaf() override;
	QList<FileLocation> getMantaEvidenceFiles() override;

	QList<FileLocation> getAnalysisLogFiles() override;
	QList<FileLocation> getCircosPlotFiles() override;
	QList<FileLocation> getVcfGzFiles() override;
	QList<FileLocation> getExpansionhunterVcfFiles() override;
	QList<FileLocation> getPrsTsvFiles() override;
	QList<FileLocation> getClincnvTsvFiles() override;
	QList<FileLocation> getLowcovBedFiles() override;
	QList<FileLocation> getStatLowcovBedFiles() override;
	QList<FileLocation> getCnvsClincnvSegFiles() override;
	QList<FileLocation> getCnvsClincnvTsvFiles() override;
	QList<FileLocation> getCnvsSegFiles() override;
	QList<FileLocation> getCnvsTsvFiles() override;
	QList<FileLocation> getRohsTsvFiles() override;

	QString getAnalysisPath() override;
	QString getProjectPath() override;
	QString getRohFileAbsolutePath() override;

	QString processedSampleName() override;

private:
	void setIsFoundFlag(FileLocation& file);
	QList<FileLocation> requestFileInfoByType(PathType type);
	FileLocation mapJsonObjectToFileLocation(QJsonObject obj);
	QList<FileLocation> mapJsonArrayToFileLocationList(QJsonArray array);

protected:
	QString sample_id_;
	QString server_host_;
	int server_port_;
};

#endif // FILELOCATIONPROVIDERSERVER_H
