#include "QueuingEngineControllerGeneric.h"
#include "HttpRequestHandler.h"
#include "ProxyDataService.h"
#include "Log.h"
#include "Settings.h"

QueuingEngineControllerGeneric::QueuingEngineControllerGeneric()
{
	proxy_ = QNetworkProxy::NoProxy;
	if(!ProxyDataService::isConnected())
	{
		const QNetworkProxy& proxy = ProxyDataService::getProxy();
		if(proxy.type() != QNetworkProxy::HttpProxy)
		{
			Log::error("No connection to the internet! Please check your proxy settings.");
			return;
		}
		if(proxy.hostName().isEmpty() || (proxy.port() < 1))
		{
			Log::error("HTTP proxy without reqired host name or port provided!");
			return;
		}

		//final check of the connection
		if(!ProxyDataService::isConnected())
		{
			Log::error("No connection to the internet! Please check your proxy settings.");
			return;
		}
	}

	// set proxy for the file download, if needed
	proxy_ = ProxyDataService::getProxy();
	if (proxy_!=QNetworkProxy::NoProxy)
	{
		Log::info("Using Proxy: " + proxy_.hostName());
	}

	qe_api_base_url_ = Settings::string("qe_api_base_url", true);
}

QString QueuingEngineControllerGeneric::getEngineName() const
{
	return "Generic";
}

void QueuingEngineControllerGeneric::submitJob(NGSD &/*db*/, int threads, QStringList queues, QStringList pipeline_args, QString project_folder, QString script, int job_id) const
{
	if (qe_api_base_url_.isEmpty())
	{
		Log::error("Could not submit a job: base URL for queuing engine API had not been set");
		return;
	}

	try
	{
		int status_code = QueuingEngineApiHelper(qe_api_base_url_, proxy_).submitJob(threads, queues, pipeline_args, project_folder, script, job_id);
		if (status_code!=200)
		{
			Log::error("There has been an error while submitting a job, response code: " + QString::number(status_code));
		}
	}
	catch(Exception e)
	{
		Log::error("There has been an error while submitting a job: " + e.message());
	}
}

bool QueuingEngineControllerGeneric::updateRunningJob(NGSD &/*db*/, const AnalysisJob &job, int job_id) const
{
	bool job_finished = true;
	if (qe_api_base_url_.isEmpty())
	{
		Log::error("Could not update a running job: base URL for queuing engine API had not been set");
		return job_finished;
	}

	try
	{
		int status_code = QueuingEngineApiHelper(qe_api_base_url_, proxy_).updateRunningJob(job.sge_id, job.sge_queue, job_id);
		if (status_code==200)
		{
			job_finished = true; // OK - the job is finished
		}
		else if (status_code==201) // CREATED - the job is queued/running
		{
			job_finished = false;
		}
		else
		{
			Log::error("There has been an error while updating a job, response code: " + QString::number(status_code));
		}
	}
	catch(Exception e)

	{
		Log::error("There has been an error while updating a job: " + e.message());
	}

	return job_finished;
}

void QueuingEngineControllerGeneric::checkCompletedJob(NGSD &/*db*/, QString qe_job_id, QByteArrayList stdout_stderr, int job_id) const
{
	if (qe_api_base_url_.isEmpty())
	{
		Log::error("Could not check a completed job: base URL for queuing engine API had not been set");
		return;
	}

	try
	{
		int status_code = QueuingEngineApiHelper(qe_api_base_url_, proxy_).checkCompletedJob(qe_job_id, stdout_stderr, job_id);
		if (status_code!=200)
		{
			Log::error("There has been an error while checking a completed job, response code: " + QString::number(status_code));
		}
	}
	catch(Exception e)
	{
		Log::error("There has been an error while checking a completed job: " + e.message());
	}
}

void QueuingEngineControllerGeneric::deleteJob(NGSD &/*db*/, const AnalysisJob &job, int job_id) const
{
	if (qe_api_base_url_.isEmpty())
	{
		Log::error("Could not delete a job: base URL for queuing engine API had not been set");
		return;
	}

	try
	{
		int status_code = QueuingEngineApiHelper(qe_api_base_url_, proxy_).deleteJob(job.sge_id, job.type, job_id);
		if (status_code!=200)
		{
			Log::error("There has been an error while deleting a job, response code: " + QString::number(status_code));
		}
	}
	catch(Exception e)
	{
		Log::error("There has been an error while deleting a job: " + e.message());
	}
}
