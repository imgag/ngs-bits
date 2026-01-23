#include "BackgroundJobDialog.h"
#include <QDateTime>
#include "GUIHelper.h"
#include "Helper.h"

BackgroundJobDialog::BackgroundJobDialog(QWidget* parent)
	: QDialog(parent)
	, ui_()
	, pool_()
	, next_id_(0)
{
	ui_.setupUi(this);

	pool_.setMaxThreadCount(3);
}

int BackgroundJobDialog::start(BackgroundWorkerBase* job, bool show_busy_dialog)
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
		job_info.busy_dlg = new BusyDialog(job->name(), this);
		job_info.busy_dlg->init("Processing...", false);
		job_info.busy_dlg->show();
	}
	jobs_.append(job_info);
	updateTable(job->id());

	//connect
	connect(job, SIGNAL(started()), this, SLOT(started()));
	connect(job, SIGNAL(finished()), this, SLOT(finished()));
	connect(job, SIGNAL(failed()), this, SLOT(failed()));

	//start
	pool_.start(job);
    return job_info.id;
}

QString BackgroundJobDialog::getJobStatus(int job_id)
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

QString BackgroundJobDialog::getJobMessages(int job_id)
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

void BackgroundJobDialog::started()
{	
	BackgroundWorkerBase* worker = qobject_cast<BackgroundWorkerBase*>(sender());
	if (worker==nullptr) THROW(ProgrammingException, "BackgroundJobDialog::started called by Qobject that is not a BackgroundWorkerBase!");

	for (int r=0; r<jobs_.count(); ++r)
	{
		if (jobs_[r].id==worker->id())
		{
			jobs_[r].status = "started";
		}
	}
	updateTable(worker->id());
}

void BackgroundJobDialog::finished()
{
	BackgroundWorkerBase* worker = qobject_cast<BackgroundWorkerBase*>(sender());
	if (worker==nullptr) THROW(ProgrammingException, "BackgroundJobDialog::finished called by Qobject that is not a BackgroundWorkerBase!");

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
	updateTable(worker->id());

	sender()->deleteLater();
}

void BackgroundJobDialog::failed()
{
	BackgroundWorkerBase* worker = qobject_cast<BackgroundWorkerBase*>(sender());
	if (worker==nullptr) THROW(ProgrammingException, "BackgroundJobDialog::failed called by Qobject that is not a BackgroundWorkerBase!");

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
	updateTable(worker->id());

	sender()->deleteLater();
}

void BackgroundJobDialog::updateTable(int id)
{
	//resize table
	ui_.jobs->setRowCount(jobs_.count());

	for (int r=0; r<jobs_.count(); ++r)
	{
		const JobInfo& job_info = jobs_[r];
		if (id!=-1 && job_info.id!=id) continue;

		ui_.jobs->setItem(r, 0, GUIHelper::createTableItem(job_info.name));

		ui_.jobs->setItem(r, 1, GUIHelper::createTableItem(Helper::toString(job_info.started, ' ')));

		QTableWidgetItem* item = GUIHelper::createTableItem(job_info.status);
        if (job_info.status=="queued") item->setBackground(QBrush(QColor(Qt::lightGray)));
        else if (job_info.status=="started") item->setBackground(QBrush(QColor("#90EE90")));
        else if (job_info.status=="finished") item->setBackground(QBrush(QColor("#44BB44")));
        else if (job_info.status=="failed") item->setBackground(QBrush(QColor("#FF0000")));
		ui_.jobs->setItem(r, 2, item);


		ui_.jobs->setItem(r, 3, GUIHelper::createTableItem(job_info.elapsed_ms==-1 ? "" : Helper::elapsedTime(job_info.elapsed_ms)));

		ui_.jobs->setItem(r, 4, GUIHelper::createTableItem(job_info.messages));
	}

	GUIHelper::resizeTableCellWidths(ui_.jobs, 400);
	ui_.jobs->resizeRowsToContents();
}
