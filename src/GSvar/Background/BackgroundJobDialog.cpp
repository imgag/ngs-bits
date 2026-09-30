#include "BackgroundJobDialog.h"
#include "Background/BackgroundJobController.h"
#include <QDateTime>
#include "GUIHelper.h"
#include "Helper.h"

BackgroundJobDialog::BackgroundJobDialog(QWidget* parent)
	: QDialog(parent)
	, ui_()
	, job_controller_(BackgroundJobController::instance())
{
	ui_.setupUi(this);

	connect(&job_controller_, SIGNAL(jobChanged(int)), this, SLOT(updateTable(int)));
	connect(&job_controller_, SIGNAL(showBusyDialog(QString, BusyDialog*)), this, SLOT(showBusyDialog(QString, BusyDialog*)));
}

void BackgroundJobDialog::showBusyDialog(QString name, BusyDialog* jobinfo_dlg)
{
	jobinfo_dlg = new BusyDialog(name, this);
	jobinfo_dlg->init("Processing...", false);
	jobinfo_dlg->show();
}

void BackgroundJobDialog::updateTable(int id)
{
	//resize table
	ui_.jobs->setRowCount(job_controller_.getJobs().count());

	for (int r=0; r<job_controller_.getJobs().count(); ++r)
	{
		const JobInfo& job_info = job_controller_.getJobs()[r];
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
