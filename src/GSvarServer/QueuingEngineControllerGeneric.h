#ifndef QUEUINGENGINECONTROLLERGENERIC_H
#define QUEUINGENGINECONTROLLERGENERIC_H

#include "QueuingEngineController.h"
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
	void submitJob(NGSD& db, int threads, QStringList queues, QStringList pipeline_args, QString project_folder, QString script, int job_id) const override;
	bool updateRunningJob(NGSD& db, const AnalysisJob &job, int job_id) const override;
	void checkCompletedJob(NGSD& db, QString qe_job_id, QByteArrayList stdout_stderr, int job_id) const override;
	void deleteJob(NGSD &db, const AnalysisJob &job, int job_id) const override;

private:
	QNetworkProxy proxy_;
	QString qe_api_base_url_;
	static QJsonObject convertAnalysisJobToJson(const AnalysisJob job);
};


#endif // QUEUINGENGINECONTROLLERGENERIC_H
