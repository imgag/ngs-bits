#include "AnalysisStatusDialog.h"
#include "MultiSampleDialog.h"
#include "TrioDialog.h"
#include "SingleSampleAnalysisDialog.h"
#include "GUIHelper.h"
#include <QMenu>
#include <QFileInfo>
#include <QDesktopServices>
#include <QUrl>
#include <QMessageBox>

AnalysisStatusDialog::AnalysisStatusDialog(QWidget *parent)
	: QDialog(parent)
	, ui_()
	, db_()
{
	//setup UI
	ui_.setupUi(this);
	connect(ui_.analyses, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(showContextMenu(QPoint)));
	connect(ui_.analyses->selectionModel(), SIGNAL(selectionChanged(QItemSelection,QItemSelection)), this, SLOT(updateDetails()));
	connect(ui_.history, SIGNAL(itemDoubleClicked(QTableWidgetItem*)), this, SLOT(showOutputDetails(QTableWidgetItem*)));
	connect(ui_.refresh, SIGNAL(clicked(bool)), this, SLOT(refreshStatus()));
	ui_.f_date->setDate(QDate::currentDate().addMonths(-1));
	connect(ui_.analysisSingle, SIGNAL(clicked(bool)), this, SLOT(analyzeSingleSamples()));
	connect(ui_.analysisTrio, SIGNAL(clicked(bool)), this, SLOT(analyzeTrio()));
	connect(ui_.analysisMulti, SIGNAL(clicked(bool)), this, SLOT(analyzeMultiSample()));

	//misc
	refreshStatus();
}

void AnalysisStatusDialog::analyzeSingleSamples(QList<AnalysisJobSample> samples)
{
	SingleSampleAnalysisDialog dlg(this);
	dlg.setSamples(samples);
	if (dlg.exec()==QDialog::Accepted)
	{
		foreach(AnalysisJobSample sample,  dlg.samples())
		{
			db_.queueAnalysis("single sample", dlg.highPriority(), dlg.arguments(), QList<AnalysisJobSample>() << sample);
		}
	}

	refreshStatus();
}

void AnalysisStatusDialog::analyzeTrio(QList<AnalysisJobSample> samples)
{
	TrioDialog dlg(this);
	dlg.setSamples(samples);
	if (dlg.exec()==QDialog::Accepted)
	{
		db_.queueAnalysis("trio", dlg.highPriority(), dlg.arguments(), dlg.samples());
	}

	refreshStatus();
}

void AnalysisStatusDialog::analyzeMultiSample(QList<AnalysisJobSample> samples)
{
	MultiSampleDialog dlg(this);
	dlg.setSamples(samples);
	if (dlg.exec()==QDialog::Accepted)
	{
		db_.queueAnalysis("multi sample", dlg.highPriority(), dlg.arguments(), dlg.samples());
	}

	refreshStatus();
}

void AnalysisStatusDialog::refreshStatus()
{
	//query job IDs
	SqlQuery query = db_.getQuery();
	query.exec("SELECT id FROM analysis_job ORDER BY ID DESC");

	//clear contents
	ui_.analyses->clearContents();
	ui_.analyses->setRowCount(0);
	jobs_.clear();

	//add lines
	int row = -1;
	QSet<QString> tags_seen;
	while(query.next())
	{
		//get job infos
		int job_id = query.value(0).toInt();
		AnalysisJob job = db_.analysisInfo(job_id);

		//check if job was already seen (i.e. it was repeated)
		QStringList tag_parts;
		tag_parts << job.type;
		foreach(const AnalysisJobSample& sample, job.samples)
		{
			tag_parts << sample.name + "|" + sample.info;
		}
		std::sort(tag_parts.begin(), tag_parts.end());
		QString tag = tag_parts.join(" ");
		bool repeated = tags_seen.contains(tag);
		tags_seen.insert(tag);

		//filter repeated
		if (repeated && !ui_.f_repeated->isChecked()) continue;

		//filter sample
		QString f_sample = ui_.f_sample->text().trimmed();
		if (!f_sample.isEmpty())
		{
			bool skip = true;
			foreach(const AnalysisJobSample& sample, job.samples)
			{
				if (sample.name.contains(f_sample)) skip = false;
			}
			if (skip) continue;
		}

		//filter user
		QString f_user = ui_.f_user->text().trimmed();
		if (!f_user.isEmpty())
		{
			bool skip = true;
			if (job.history.count()>0 && job.history[0].user.contains(f_user)) skip = false;
			if (skip) continue;
		}

		//filter date
		QDateTime f_date = QDateTime(ui_.f_date->date());
		bool skip = true;
		if (job.history.count()>0 && job.history[0].time>=f_date) skip = false;
		if (skip) break; //jobs are ordered by date => no newer jobs can come => skip the rest

		//not filtered => add row
		++row;
		ui_.analyses->setRowCount(ui_.analyses->rowCount()+1);
		jobs_ << JobData{job_id, job, repeated};

		//queued
		if (job.history.count()>0)
		{
			addItem(ui_.analyses, row, 0, job.history[0].timeAsString());
			addItem(ui_.analyses, row, 1, job.history[0].user);
		}

		//type
		addItem(ui_.analyses, row, 2, job.type);

		//sample(s)
		QList<ProcessedSampleData> ps_data;
		QStringList parts;
		foreach(const AnalysisJobSample& sample, job.samples)
		{
			parts << sample.name;
			ps_data << db_.getProcessedSampleData(db_.processedSampleId(sample.name));
		}
		addItem(ui_.analyses, row, 3, parts.join(" "));

		//system(s)
		parts.clear();
		foreach(const ProcessedSampleData& data, ps_data)
		{
			parts << data.processing_system;
		}
		parts.removeDuplicates();
		addItem(ui_.analyses, row, 4, parts.count()==1 ? parts[0] : "");

		//run(s)
		parts.clear();
		foreach(const ProcessedSampleData& data, ps_data)
		{
			parts << data.run_name;
		}
		parts.removeDuplicates();
		addItem(ui_.analyses, row, 5, parts.count()==1 ? parts[0] : "");

		//project(s)
		parts.clear();
		foreach(const ProcessedSampleData& data, ps_data)
		{
			parts << data.project_name;
		}
		parts.removeDuplicates();
		addItem(ui_.analyses, row, 6, parts.count()==1 ? parts[0] : "");

		//status
		QString status = job.finalStatus();
		QColor bg = statusToColor(status);
		if (repeated)
		{
			bg = QColor("#D3D3D3");
			status += " > repeated";
		}

		addItem(ui_.analyses, row, 7, status, bg);
	}

	GUIHelper::resizeTableCells(ui_.analyses, 400);
}

void AnalysisStatusDialog::showContextMenu(QPoint pos)
{
	//extract selected rows
	QList<int> rows;
	auto ranges = ui_.analyses->selectedRanges();
	foreach(auto range, ranges)
	{
		for (int i=range.topRow(); i<=range.bottomRow(); ++i)
		{
			rows << i;
		}
	}
	std::sort(rows.begin(), rows.end());

	//nothing selected => skip
	if (rows.isEmpty()) return;

	//extract processed sample data
	QList<AnalysisJobSample> samples;
	QSet<QString> types;
	QSet<int> job_ids;
	bool all_running = true;
	bool all_finished = true;
	foreach(int row, rows)
	{
		foreach(const AnalysisJobSample& sample, jobs_[row].job_data.samples)
		{
			if (!samples.contains(sample))
			{
				samples << sample;
			}
		}

		job_ids << jobs_[row].ngsd_id;
		types << jobs_[row].job_data.type;

		if (jobs_[row].job_data.isRunning())
		{
			all_finished = false;
		}
		else
		{
			all_running = false;
		}
	}

	//set up menu
	QMenu menu;
	menu.addAction(QIcon(":/Icons/NGSD.png"), "Open sample in NGSD");
	menu.addAction(QIcon(":/Icons/Folder.png"), "Open sample folder");
	if (all_running)
	{
		menu.addAction(QIcon(":/Icons/Remove.png"), "Cancel");
	}
	if (all_finished && types.count()==1)
	{
		QString type = types.values()[0];
		if (type=="single sample")
		{
			menu.addAction(QIcon(":/Icons/Refresh.png"), "Restart single sample analysis");
		}
		else if (type=="multi sample" && job_ids.count()==1)
		{
			menu.addAction(QIcon(":/Icons/Refresh.png"), "Restart multi-sample analysis");
		}
		else if (type=="trio" && job_ids.count()==1)
		{
			menu.addAction(QIcon(":/Icons/Refresh.png"), "Restart trio analysis");
		}
	}

	//show menu
	QAction* action = menu.exec(ui_.analyses->viewport()->mapToGlobal(pos));
	if (action==nullptr) return;

	//execute
	QString text = action->text();
	if (text=="Open sample in NGSD")
	{
		foreach(AnalysisJobSample sample, samples)
		{
			QDesktopServices::openUrl(QUrl(db_.url(sample.name)));
		}
	}
	if (text=="Open sample folder")
	{
		foreach(AnalysisJobSample sample, samples)
		{
			QDesktopServices::openUrl(db_.processedSamplePath(db_.processedSampleId(sample.name), NGSD::FOLDER));
		}
	}
	if (text=="Cancel")
	{
		foreach(int id, job_ids)
		{
			db_.cancelAnalysis(id);
		}
		refreshStatus();
	}
	if (text=="Restart single sample analysis")
	{
		analyzeSingleSamples(samples);
		refreshStatus();
	}
	if (text=="Restart multi-sample analysis")
	{
		analyzeMultiSample(samples);
		refreshStatus();
	}
	if (text=="Restart trio analysis")
	{
		analyzeTrio(samples);
		refreshStatus();
	}
}

void AnalysisStatusDialog::clearDetails()
{
	//details
	ui_.properties->clearContents();
	ui_.properties->setRowCount(0);

	//samples
	ui_.samples->clearContents();
	ui_.samples->setRowCount(0);

	//history
	ui_.history->clearContents();
	ui_.history->setRowCount(0);
}

void AnalysisStatusDialog::updateDetails()
{
	//clear selection
	clearDetails();

	//determine row
	auto ranges = ui_.analyses->selectedRanges();
	if (ranges.count()!=1 || ranges[0].rowCount()!=1) return;

	int selection_row = ranges[0].topRow();
	const AnalysisJob& job = jobs_[selection_row].job_data;

	//properties
	ui_.properties->setRowCount(5);
	addItem(ui_.properties, 0, 0, "high_priority");
	addItem(ui_.properties, 0, 1, job.high_priority ? "yes" : "no");
	addItem(ui_.properties, 1, 0, "arguments");
	addItem(ui_.properties, 1, 1, job.args);
	addItem(ui_.properties, 2, 0, "SGE id");
	addItem(ui_.properties, 2, 1, job.sge_id);
	addItem(ui_.properties, 3, 0, "SGE queue");
	addItem(ui_.properties, 3, 1, job.sge_queue);
	addItem(ui_.properties, 4, 0, "run time");
	addItem(ui_.properties, 4, 1, job.runTimeAsString());
	GUIHelper::resizeTableCells(ui_.properties);

	//samples
	ui_.samples->setRowCount(job.samples.count());
	int r = 0;
	foreach(const AnalysisJobSample& sample, job.samples)
	{
		addItem(ui_.samples, r, 0, sample.name);
		addItem(ui_.samples, r, 1, sample.info);
		++r;
	}
	GUIHelper::resizeTableCells(ui_.samples);

	//history
	ui_.history->setRowCount(job.history.count());
	r = 0;
	foreach(const AnalysisJobHistoryEntry& entry, job.history)
	{
		addItem(ui_.history, r, 0, entry.timeAsString());
		addItem(ui_.history, r, 1, entry.user);
		addItem(ui_.history, r, 2, entry.status, statusToColor(entry.status));
		QString output = entry.output.join("\n").trimmed();
		QColor bg = Qt::transparent;
		if (output.contains("warning", Qt::CaseInsensitive)) bg =  QColor("#FFC45E");
		if (output.contains("error", Qt::CaseInsensitive)) bg =  QColor("#FF0000");
		if (entry.output.count()>1)
		{
			addItem(ui_.history, r, 3, "double-click for details", bg);
			ui_.history->item(r, 3)->setData(Qt::UserRole, output);
		}
		else
		{
			addItem(ui_.history, r, 3, output, bg);
		}
		++r;
	}
	GUIHelper::resizeTableCells(ui_.history);
}

void AnalysisStatusDialog::showOutputDetails(QTableWidgetItem* item)
{
	if (item==nullptr) return;

	QStringList output = item->data(Qt::UserRole).toStringList();
	if (output.empty()) return;

	QMessageBox::information(this, "Output", output.join("\n"));
}

void AnalysisStatusDialog::addItem(QTableWidget* table, int row, int col, QString text, QColor bg_color)
{
	auto item = new QTableWidgetItem(text);
	item->setFlags(Qt::ItemIsSelectable|Qt::ItemIsEnabled);
	item->setBackgroundColor(bg_color);
	table->setItem(row, col, item);
}

QColor AnalysisStatusDialog::statusToColor(QString status)
{
	QColor output = Qt::transparent;
	if (status=="started") output = QColor("#90EE90");
	if (status=="finished") output = QColor("#44BB44");
	if (status=="canceled") output = QColor("#FFC45E");
	if (status=="error") output = QColor("#FF0000");
	return output;
}
