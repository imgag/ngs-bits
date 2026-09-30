
#include "BackgroundJobController.h"
#include "Exceptions.h"


BackgroundJobController::BackgroundJobController()
	: QObject()
	, pool_()
	, next_id_(0)
{
	pool_.setMaxThreadCount(3);
}

BackgroundJobController& BackgroundJobController::instance()
{
	static BackgroundJobController instance;

	return instance;
}

int BackgroundJobController::start(BackgroundWorkerBase* job, bool show_busy_dialog)
{
	//set ID
	job->setId(next_id_);
	++next_id_;

	//add to table
	JobInfo job_info;
	job_info.id = job->id();
	job_info.name = job->name();
	job_info.started = QDateTime::currentDateTime();
	job_info.status = "queued";
	if (show_busy_dialog)
	{
		emit showBusyDialog(job->name(), job_info.busy_dlg);
	}
	jobs_.append(job_info);
	emit jobChanged(job->id());

	//connect
	connect(job, SIGNAL(started()), this, SLOT(started()));
	connect(job, SIGNAL(finished()), this, SLOT(finished()));
	connect(job, SIGNAL(failed()), this, SLOT(failed()));

	//start
	pool_.start(job);
	return job_info.id;
}

QString BackgroundJobController::getJobStatus(int job_id)
{
	for (int i=0; i<jobs_.count(); ++i)
	{
		if (jobs_[i].id==job_id)
		{
			return jobs_[i].status;
		}
	}
	return "";
}

QString BackgroundJobController::getJobMessages(int job_id)
{
	for (int i=0; i<jobs_.count(); ++i)
	{
		if (jobs_[i].id==job_id)
		{
			return jobs_[i].messages;
		}
	}
	return "";
}

const QList<JobInfo>& BackgroundJobController::getJobs()
{
	return jobs_;
}


void BackgroundJobController::started()
{
	BackgroundWorkerBase* worker = qobject_cast<BackgroundWorkerBase*>(sender());
	if (worker==nullptr) THROW(ProgrammingException, "BackgroundJobController::started called by Qobject that is not a BackgroundWorkerBase!");

	for (int r=0; r<jobs_.count(); ++r)
	{
		if (jobs_[r].id==worker->id())
		{
			jobs_[r].status = "started";
		}
	}
	emit jobChanged(worker->id());
}

void BackgroundJobController::finished()
{
	BackgroundWorkerBase* worker = qobject_cast<BackgroundWorkerBase*>(sender());
	if (worker==nullptr) THROW(ProgrammingException, "BackgroundJobController::finished called by Qobject that is not a BackgroundWorkerBase!");

	for (int r=0; r<jobs_.count(); ++r)
	{
		if (jobs_[r].id==worker->id())
		{
			jobs_[r].status = "finished";
			jobs_[r].elapsed_ms = worker->elapsed();

			//stop busy dialog
			if (jobs_[r].busy_dlg!=nullptr)
			{
				jobs_[r].busy_dlg->hide();
				jobs_[r].busy_dlg->deleteLater();
			}

			//user interaction
			worker->userInteration();
		}
	}
	emit jobChanged(worker->id());

	sender()->deleteLater();
}

void BackgroundJobController::failed()
{
	BackgroundWorkerBase* worker = qobject_cast<BackgroundWorkerBase*>(sender());
	if (worker==nullptr) THROW(ProgrammingException, "BackgroundJobController::failed called by Qobject that is not a BackgroundWorkerBase!");

	for (int r=0; r<jobs_.count(); ++r)
	{
		if (jobs_[r].id==worker->id())
		{
			jobs_[r].status = "failed";
			jobs_[r].elapsed_ms = worker->elapsed();
			jobs_[r].messages = worker->error();

			//stop busy dialog
			if (jobs_[r].busy_dlg!=nullptr)
			{
				jobs_[r].busy_dlg->hide();
				jobs_[r].busy_dlg->deleteLater();
			}

			//user interaction
			worker->userInteration();
		}
	}
	emit jobChanged(worker->id());

	sender()->deleteLater();
}