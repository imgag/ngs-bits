#include "BackgroundJobDialog.h"
#include <QDateTime>
#include "GUIHelper.h"
#include "Helper.h"

BackgroundJobDialog::BackgroundJobDialog(QWidget* parent)
	:QDialog(parent)
	, ui_()
	, pool_()
	, next_id_(0)
{
	ui_.setupUi(this);

	pool_.setMaxThreadCount(1);
}

void BackgroundJobDialog::start(BackgroundWorkerBase* job)
{
	//set ID
	job->setId(next_id_);
	++next_id_;

	//add to table
	int r = ui_.jobs->rowCount();
	ui_.jobs->setRowCount(r+1);
	ui_.jobs->setItem(r, 0, GUIHelper::createTableItem(job->name()));
	ui_.jobs->setItem(r, 1, GUIHelper::createTableItem(QDateTime::currentDateTime().toString(Qt::ISODate).replace('T', ' ')));

	QTableWidgetItem* item = GUIHelper::createTableItem("queued");
	item->setBackgroundColor(Qt::lightGray);
	ui_.jobs->setItem(r, 2, item);
	updateTableWidths();

	//connect
	connect(job, SIGNAL(started(int)), this, SLOT(started(int)));
	connect(job, SIGNAL(finished(int, int)), this, SLOT(finished(int, int)));
	connect(job, SIGNAL(failed(int, int, QString)), this, SLOT(failed(int, int, QString)));

	//start
	pool_.start(job);
}

void BackgroundJobDialog::started(int id)
{
	QTableWidgetItem* item = GUIHelper::createTableItem("started");
	item->setBackgroundColor("#90EE90");
	ui_.jobs->setItem(id, 2, item);
	updateTableWidths();
}

void BackgroundJobDialog::finished(int id, int elapsed_ms)
{
	QTableWidgetItem* item = GUIHelper::createTableItem("finished");
	item->setBackgroundColor("#44BB44");
	ui_.jobs->setItem(id, 2, item);
	ui_.jobs->setItem(id, 3, GUIHelper::createTableItem(Helper::elapsedTime(elapsed_ms)));
	updateTableWidths();
}

void BackgroundJobDialog::failed(int id, int elapsed_ms, QString error)
{
	QTableWidgetItem* item = GUIHelper::createTableItem("failed");
	item->setBackgroundColor("#FF0000");
	ui_.jobs->setItem(id, 2, item);
	ui_.jobs->setItem(id, 3, GUIHelper::createTableItem(Helper::elapsedTime(elapsed_ms)));
	ui_.jobs->setItem(id, 4, GUIHelper::createTableItem(error));
	updateTableWidths();
}

void BackgroundJobDialog::updateTableWidths()
{
	GUIHelper::resizeTableCells(ui_.jobs, 400);
}
