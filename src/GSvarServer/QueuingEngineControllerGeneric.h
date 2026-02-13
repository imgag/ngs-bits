#ifndef QUEUINGENGINECONTROLLERGENERIC_H
#define QUEUINGENGINECONTROLLERGENERIC_H

#include "QueuingEngineController.h"
#include "QueuingEngineApiHelper.h"
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>

class QueuingEngineControllerGeneric
	: public QueuingEngineController
{
public:
	QueuingEngineControllerGeneric();

protected:
	QString getEngineName() const override;
	void submitJob(NGSD& db, int threads, QStringList queues, QStringList pipeline_args, QString working_directory, QString script, int job_id) const override;
	bool updateRunningJob(NGSD& db, const AnalysisJob &job, int job_id) const override;
	void checkCompletedJob(NGSD& db, QString qe_job_id, QByteArrayList stdout_stderr, int job_id) const override;
	void deleteJob(NGSD &db, const AnalysisJob &job, int job_id) const override;

	bool hasApiUrl() const;
	bool checkReplyIsValid(QJsonDocument &reply_doc, int job_id, QByteArray action) const;
	QByteArrayList getResults(QJsonDocument &reply_doc) const;
	QString getJobId(QJsonDocument &reply_doc, bool &ok) const;
	QString getStatus(QJsonDocument &reply_doc, bool &ok) const;
	int getCommandExitCode(QJsonDocument &reply_doc) const;
	int getEngineExitCode(QJsonDocument &reply_doc, bool &ok) const;
	QString getQueue(QJsonDocument &reply_doc, bool &ok) const;


private:
	QNetworkProxy proxy_;
	QString qe_api_base_url_;
	QString secure_token_;
};


#endif // QUEUINGENGINECONTROLLERGENERIC_H
