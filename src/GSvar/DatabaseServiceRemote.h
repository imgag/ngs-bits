#ifndef DATABASESERVICEREMOTE_H
#define DATABASESERVICEREMOTE_H

#include "DatabaseService.h"
#include "Exceptions.h"
#include "LoginManager.h"
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>

class DatabaseServiceRemote
	: virtual public DatabaseService
{
public:
	DatabaseServiceRemote();
	virtual ~DatabaseServiceRemote() {}

	virtual bool enabled() const override;
	virtual QString checkPassword(const QString user_name, const QString password) const override;

	virtual BedFile processingSystemRegions(int sys_id, bool ignore_if_missing) const override;
	virtual BedFile processingSystemAmplicons(int sys_id, bool ignore_if_missing) const override;
	virtual GeneSet processingSystemGenes(int sys_id, bool ignore_if_missing) const override;
	virtual QStringList secondaryAnalyses(QString processed_sample_name, QString analysis_type) const override;

	virtual FileLocation processedSamplePath(const QString& processed_sample_id, PathType type) const override;
	virtual FileInfo analysisJobLatestLogInfo(const int& job_id) const override;
	virtual FileLocation analysisJobGSvarFile(const int& job_id) const override;
	virtual FileLocation analysisJobLogFile(const int& job_id) const override;

protected:	
	QByteArray makeGetApiCall(QString api_path, RequestUrlParams params, bool ignore_if_missing) const;
	QByteArray makePostApiCall(QString api_path, RequestUrlParams params, QByteArray content, bool ignore_if_missing) const;

	//Throws an error if NGSD is not enabled
	void checkEnabled(QString function) const
	{
		if (!enabled_)
		{
			THROW(ProgrammingException, "NGSD is not enabled, but instance requested in '" + function + "'");
		}
	}

	bool enabled_;
};

#endif // DATABASESERVICEREMOTE_H
