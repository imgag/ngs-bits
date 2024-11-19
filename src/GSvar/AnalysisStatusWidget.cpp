#include "AnalysisStatusWidget.h"
#include "MultiSampleDialog.h"
#include "TrioDialog.h"
#include "SomaticDialog.h"
#include "SingleSampleAnalysisDialog.h"
#include "GUIHelper.h"
#include "cmath"
#include "LoginManager.h"
#include "GlobalServiceProvider.h"
#include "AnalysisInformationWidget.h"
#include "GSvarHelper.h"
#include "ClientHelper.h"
#include <QMenu>
#include <QFileInfo>
#include <QDesktopServices>
#include <QUrl>
#include <QDir>
#include <QMessageBox>
#include <QMetaMethod>
#include <QProcess>

AnalysisStatusWidget::AnalysisStatusWidget(QWidget* parent)
	: QWidget(parent)
	, ui_()
{
	//setup UI
	ui_.setupUi(this);
	GUIHelper::styleSplitter(ui_.splitter);
	connect(ui_.analyses, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(showContextMenu(QPoint)));
	connect(ui_.analyses, SIGNAL(cellDoubleClicked(int,int)), this, SLOT(openProcessedSampleTab(int,int)));
	connect(ui_.analyses->selectionModel(), SIGNAL(selectionChanged(QItemSelection,QItemSelection)), this, SLOT(updateDetails()));
	connect(ui_.refresh, SIGNAL(clicked(bool)), this, SLOT(refreshStatus()));
	ui_.f_date->setDate(QDate::currentDate().addDays(-7));
	connect(ui_.analysisSingle, SIGNAL(clicked(bool)), this, SLOT(analyzeSingleSamples()));
	connect(ui_.analysisTrio, SIGNAL(clicked(bool)), this, SLOT(analyzeTrio()));
	connect(ui_.analysisMulti, SIGNAL(clicked(bool)), this, SLOT(analyzeMultiSample()));
	connect(ui_.analysisSomatic, SIGNAL(clicked(bool)), this, SLOT(analyzeSomatic()));
	connect(ui_.copy_btn, SIGNAL(clicked(bool)), this, SLOT(copyToClipboard()));
	connect(ui_.f_text, SIGNAL(returnPressed()), this, SLOT(applyFilters()));
	connect(ui_.f_mine, SIGNAL(stateChanged(int)), this, SLOT(applyFilters()));
}

bool AnalysisStatusWidget::updateIsRunning() const
{
	return update_running_;
}

void AnalysisStatusWidget::analyzeSingleSamples(QList<AnalysisJobSample> samples)
{
	if (GSvarHelper::queueSampleAnalysis(AnalysisType::GERMLINE_SINGLESAMPLE, samples, this))
	{
		refreshStatus();
	}
}

void AnalysisStatusWidget::analyzeTrio(QList<AnalysisJobSample> samples)
{
	if (GSvarHelper::queueSampleAnalysis(AnalysisType::GERMLINE_TRIO, samples, this))
	{
		refreshStatus();
	}
}

void AnalysisStatusWidget::analyzeMultiSample(QList<AnalysisJobSample> samples)
{
	if (GSvarHelper::queueSampleAnalysis(AnalysisType::GERMLINE_MULTISAMPLE, samples, this))
	{
		refreshStatus();
	}
}

void AnalysisStatusWidget::analyzeSomatic(QList<AnalysisJobSample> samples)
{
	if (GSvarHelper::queueSampleAnalysis(AnalysisType::SOMATIC_PAIR, samples, this))
	{
		refreshStatus();
	}
}

void AnalysisStatusWidget::refreshStatus()
{
	QApplication::setOverrideCursor(Qt::BusyCursor);

	update_running_ = true;

	try
	{
		QTime timer;
		timer.start();

		//query job IDs
		NGSD db;
		SqlQuery query = db.getQuery();
		query.exec("SELECT id FROM analysis_job ORDER BY ID DESC");

		//clear contents
		ui_.analyses->clearContents();
		ui_.analyses->setRowCount(0);
		jobs_.clear();

		//set default row height (makes the table nicer while loading)
		ui_.analyses->verticalHeader()->setDefaultSectionSize(23);

		//add lines
		int row = -1;
		QSet<QString> tags_seen;
		while(query.next())
		{
			//get job infos
			int job_id = query.value(0).toInt();
			AnalysisJob job = db.analysisInfo(job_id);

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

			//filter date
			QDateTime f_date = QDateTime(ui_.f_date->date());
			if (job.history.count()==0)
			{
				qDebug() << "No history for job " << job_id;
				continue;
			}

			if (job.history[0].time<f_date) break; //jobs are ordered by date => no newer jobs can come => skip the rest

			//get sample data
			QList<ProcessedSampleData> ps_data;
			foreach(const AnalysisJobSample& sample, job.samples)
			{
				ps_data << db.getProcessedSampleData(db.processedSampleId(sample.name));
			}

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
			bool bad_quality = false;
			QStringList parts;
			foreach(const ProcessedSampleData& data, ps_data)
			{
				parts << data.name;
				if (data.quality=="bad") bad_quality = true;
			}
			QTableWidgetItem* item = addItem(ui_.analyses, row, 3, parts.join(" "));
			if (bad_quality)
			{
				item->setIcon(QIcon(":/Icons/quality_bad.png"));
				item->setToolTip("At least one sample has bad quality!");
			}

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
			QString status = job.lastStatus();
			QColor bg = statusToColor(status);
			if (job.sge_queue!="")
			{
				status += " (" + job.sge_queue + ")";
			}
			if (repeated)
			{
				bg = QColor("#D3D3D3");
				status += " > repeated";
			}
			addItem(ui_.analyses, row, 7, status, bg);

			//last update
			QString last_update;
			QColor bg_color = Qt::transparent;
			if (status.startsWith("started ("))
			{
				FileInfo info = GlobalServiceProvider::database().analysisJobLatestLogInfo(job_id);
				if (!info.isEmpty())
				{
					int sec = info.last_modiefied.secsTo(QDateTime::currentDateTime());
					if (sec>36000) bg_color = QColor("#FFC45E"); //36000s ~ 10h
					last_update = timeHumanReadable(sec) + " ago (" + info.file_name + ")";
				}
			}
			addItem(ui_.analyses, row, 8, last_update, bg_color);
		}

		//apply other filder
		applyFilters();
	}
	catch (Exception& e)
	{
		QMessageBox::warning(this, "Update failed", "Could not update data:\n" + e.message());
	}

	GUIHelper::resizeTableCellWidths(ui_.analyses, 400);
	GUIHelper::resizeTableCellHeightsToFirst(ui_.analyses);

	update_running_ = false;

	QApplication::restoreOverrideCursor();
}

void AnalysisStatusWidget::showContextMenu(QPoint pos)
{
	//extract selected rows
	QList<int> rows = selectedRows();

	//nothing selected => skip
	if (rows.isEmpty()) return;

	//extract processed sample data
	QList<AnalysisJobSample> samples;
	QStringList types;
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
		QString type = jobs_[row].job_data.type;
		if (!types.contains(type)) types << type;

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
	if (rows.count()==1 && types[0]=="single sample")
	{
		menu.addAction(QIcon(":/Icons/analysis_info.png"), "Show analysis information");
	}
	if (rows.count()==1)
	{
		menu.addAction(QIcon(":/Icons/Icon.png"), "Open variant list");
	}
	menu.addAction(QIcon(":/Icons/NGSD_sample.png"), "Open processed sample tab");
	menu.addAction(QIcon(":/Icons/NGSD_run.png"), "Open sequencing run tab");
	menu.addAction(QIcon(":/Icons/Folder.png"), "Open analysis folder(s)");
	if (types.count()==1 && types[0]=="single sample")
	{
		menu.addAction(QIcon(":/Icons/Folder.png"), "Open sample folder(s)");
	}
	menu.addAction(QIcon(":/Icons/File.png"), "Open log file");
	if (all_running)
	{
		menu.addAction(QIcon(":/Icons/Remove.png"), "Cancel");
	}
	if (all_finished && types.count()==1)
	{
		QString type = types[0];

		if (type=="single sample")
		{
			//Show restart action only if only DNA or only RNA samples are selected
			QSet<QString> sample_types;
			NGSD db;
			foreach(const AnalysisJobSample& sample, samples)
			{
				QString sample_type = db.getSampleData(db.sampleId(sample.name)).type;
				if (sample_type=="RNA") sample_types << "RNA";
				else if (sample_type.startsWith("DNA")) sample_types << "DNA";
				else if (sample_type=="cfDNA") sample_types << "cfDNA";
				else THROW(ProgrammingException, "Unhandled sample type: "+sample_type);
			}
			if((sample_types.count() == 1)) menu.addAction(QIcon(":/Icons/reanalysis.png"), "Restart single sample analysis");
		}
		else if (type=="multi sample" && job_ids.count()==1)
		{
			menu.addAction(QIcon(":/Icons/reanalysis.png"), "Restart multi-sample analysis");
		}
		else if (type=="trio" && job_ids.count()==1)
		{
			menu.addAction(QIcon(":/Icons/reanalysis.png"), "Restart trio analysis");
		}
		else if (type=="somatic" && job_ids.count()==1)
		{
			menu.addAction(QIcon(":/Icons/reanalysis.png"), "Restart somatic analysis");
		}
	}
	if (all_finished)
	{
		menu.addSeparator();
		menu.addAction(QIcon(":/Icons/Trash.png"), "Delete");
	}

	//show menu
	QAction* action = menu.exec(ui_.analyses->viewport()->mapToGlobal(pos));
	if (action==nullptr) return;

	//execute
	QString text = action->text();
	if (text=="Show analysis information")
	{
		NGSD db;
		foreach(const AnalysisJobSample& sample, samples)
		{
			QString ps = sample.name;
			AnalysisInformationWidget* widget = new AnalysisInformationWidget(db.processedSampleId(ps));
			auto dlg = GUIHelper::createDialog(widget, "Analysis information of " + ps);
			dlg->exec();
		}
	}
	if (text=="Open variant list")
	{
		NGSD db;
		foreach(int id, job_ids)
		{
			if (db.analysisInfo(id).isRunning())
			{
				QMessageBox::warning(this, "Loading error", "The job is still running");
				return;
			}
			FileLocation analysis_job_gsvar_file = GlobalServiceProvider::database().analysisJobGSvarFile(id);

			if (!analysis_job_gsvar_file.exists)
			{
				QMessageBox::warning(this, "Loading error", "The requested file does not exist");
				return;
			}
			emit loadFile(analysis_job_gsvar_file.filename);
		}
	}
	if (text=="Open processed sample tab")
	{
		foreach(const AnalysisJobSample& sample, samples)
		{
			GlobalServiceProvider::openProcessedSampleTab(sample.name);
		}
	}
	if (text=="Open sequencing run tab")
	{
		NGSD db;
		foreach(const AnalysisJobSample& sample, samples)
		{
			QString ps_id = db.processedSampleId(sample.name);
			ProcessedSampleData ps_data = db.getProcessedSampleData(ps_id);
			GlobalServiceProvider::openRunTab(ps_data.run_name);
		}
	}
	if (text=="Open analysis folder(s)")
	{
		if (ClientHelper::isClientServerMode())
		{
			QMessageBox::warning(this, "No access", "Analysis folder browsing is not available in client-server mode");
			return;
		}

		NGSD db;
		foreach(int row, rows)
		{
			QString folder = db.analysisJobFolder(jobs_[row].ngsd_id);
			if (!QDesktopServices::openUrl(folder))
			{
				QMessageBox::warning(this, "Error opening folder", "Folder could not be opened - it probably does not exist (yet):\n" + folder);
			}
		}
	}
	if (text=="Open sample folder(s)")
	{
		try
		{
			NGSD db;
			foreach(const AnalysisJobSample& sample, samples)
			{
				QString sample_folder;

				if (ClientHelper::isClientServerMode()) //client-server (allow opening folders if GSvar project paths are configured)
				{
					QString ps_id = db.processedSampleId(sample.name);
					QString project_type = db.getProcessedSampleData(ps_id).project_type;
					QString project_folder = db.projectFolder(project_type).trimmed();
					if (!project_folder.isEmpty())
					{
						sample_folder = db.processedSamplePath(ps_id, PathType::SAMPLE_FOLDER);
						if (!QDir(sample_folder).exists()) THROW(Exception, "Sample folder does not exist: " + sample_folder);
					}
				}
				else
				{
					sample_folder = GlobalServiceProvider::database().processedSamplePath(db.processedSampleId(sample.name), PathType::SAMPLE_FOLDER).filename;
				}

				QDesktopServices::openUrl(sample_folder);
			}
		}
		catch(Exception& e)
		{
			QMessageBox::information(this, "Open analysis folder", "Could not open analysis folder:\n" + e.message());
		}
	}
	if (text=="Open log file")
	{
		foreach(int row, rows)
		{
			QTableWidgetItem* item = ui_.analyses->item(row, 8);
			if (item==nullptr) continue;
			QString last_edit = item->text().trimmed();
			if (last_edit.isEmpty() || !last_edit.contains("(")) continue;

			FileLocation log_location = GlobalServiceProvider::database().analysisJobLogFile(jobs_[row].ngsd_id);

			//start in default text editor
			QString text_editor = Settings::string("text_editor", true).trimmed();
			if (!text_editor.isEmpty())
			{
				//create a local copy of the log file
				QString tmp_filename = GSvarHelper::localLogFolder() + log_location.fileName();
				QSharedPointer<QFile> tmp_file = Helper::openFileForWriting(tmp_filename);
				tmp_file->write(VersatileFile(log_location.filename).readAll());
				tmp_file->close();

				//open local file in text editor
				QProcess process;
				process.setProcessChannelMode(QProcess::MergedChannels);
				if (!process.startDetached(text_editor, QStringList() << tmp_filename))
				{
					QMessageBox::warning(this, "Error opening log file", "Log file could not be opened in editor "+text_editor+":\n" + log_location.fileName());
				}
			}
			else if (!QDesktopServices::openUrl(log_location.filename)) //start in brwoser
			{
				QMessageBox::warning(this, "Error opening log file", "Log file could not be opened:\n" + log_location.fileName());
			}
		}
	}
	if (text=="Cancel")
	{
		foreach(int id, job_ids)
		{
			NGSD db;

			//only the owner or admins can do this
			try
			{
				bool is_owner = false;
				AnalysisJob job = db.analysisInfo(id);
				if (!job.history.isEmpty() && job.history.at(0).user==LoginManager::userLogin())
				{
					is_owner = true;
				}
				if (!is_owner)
				{
					LoginManager::checkRoleIn(QStringList{"admin"});
				}
			}
			catch (Exception& /*e*/)
			{
				QMessageBox::warning(this, "Permissions error", "Only the owner of the analysis job or admins can cancel the job!");
				return;
			}

			db.cancelAnalysis(id);
		}
		refreshStatus();
	}
	if (text=="Restart single sample analysis")
	{
		analyzeSingleSamples(samples);
	}
	if (text=="Restart multi-sample analysis")
	{
		analyzeMultiSample(samples);
	}
	if (text=="Restart trio analysis")
	{
		analyzeTrio(samples);
	}
	if (text=="Restart somatic analysis")
	{
		analyzeSomatic(samples);
	}
	if (text=="Delete")
	{
		//only admins can do this
		try
		{
			LoginManager::checkRoleIn(QStringList{"admin"});
		}
		catch (Exception& e)
		{
			QMessageBox::warning(this, "Permissions error", e.message());
			return;
		}

		NGSD db;
		foreach(int id, job_ids)
		{
			db.deleteAnalysis(id);
		}
		refreshStatus();
	}
}

void AnalysisStatusWidget::clearDetails()
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

void AnalysisStatusWidget::updateDetails()
{
	clearDetails();

	//determine row
	QList<int> selected_rows = selectedRows();

	if (selected_rows.count()!=1) return;
	const AnalysisJob& job = jobs_[selected_rows[0]].job_data;

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
	GUIHelper::resizeTableCellWidths(ui_.properties);
	GUIHelper::resizeTableCellHeightsToFirst(ui_.properties);

	//samples
	ui_.samples->setRowCount(job.samples.count());
	int r = 0;
	foreach(const AnalysisJobSample& sample, job.samples)
	{
		addItem(ui_.samples, r, 0, sample.name);
		addItem(ui_.samples, r, 1, sample.info);
		++r;
	}
	GUIHelper::resizeTableCellWidths(ui_.samples);
	GUIHelper::resizeTableCellHeightsToFirst(ui_.samples);

	//history
	ui_.history->setRowCount(job.history.count());
	r = 0;
	foreach(const AnalysisJobHistoryEntry& entry, job.history)
	{
		addItem(ui_.history, r, 0, entry.timeAsString());
		addItem(ui_.history, r, 1, entry.user);
		addItem(ui_.history, r, 2, entry.status, statusToColor(entry.status));
		QString output = entry.output.join("\n").trimmed();
		if (output.count()>50)
		{
			output.resize(50);
			output += "...";
		}
		addItem(ui_.history, r, 3, output);
		++r;
	}
	GUIHelper::resizeTableCellWidths(ui_.history);
	GUIHelper::resizeTableCellHeightsToFirst(ui_.history);

	//output
	ui_.output->setText(job.history.last().output.join("\n"));
}

void AnalysisStatusWidget::copyToClipboard()
{
	GUIHelper::copyToClipboard(ui_.analyses);
}

void AnalysisStatusWidget::applyFilters()
{
	//show everything
	for (int r=0; r<ui_.analyses->rowCount(); ++r)
	{
		ui_.analyses->setRowHidden(r, false);
	}

	//text filter
	QString f_text = ui_.f_text->text().trimmed();
	if (!f_text.isEmpty())
	{
		for (int r=0; r<ui_.analyses->rowCount(); ++r)
		{
			bool match = false;
			for (int c=0; c<ui_.analyses->columnCount(); ++c)
			{
				QTableWidgetItem* item = ui_.analyses->item(r, c);
				if (item==nullptr) continue;

				if (item->text().contains(f_text, Qt::CaseInsensitive))
				{
					match = true;
					break;
				}
			}

			if (!match)
			{
				ui_.analyses->setRowHidden(r, true);
			}
		}
	}

	//user filter
	if (ui_.f_mine->isChecked())
	{
		QString f_user = Helper::userName();
		for (int r=0; r<ui_.analyses->rowCount(); ++r)
		{
			QTableWidgetItem* item = ui_.analyses->item(r, 1);
			if (item==nullptr) continue;

			if (!item->text().contains(f_user, Qt::CaseInsensitive))
			{
				ui_.analyses->setRowHidden(r, true);
			}
		}
	}

	//update column widths
	GUIHelper::resizeTableCellWidths(ui_.analyses, 350);
	GUIHelper::resizeTableCellHeightsToFirst(ui_.analyses);
}

void AnalysisStatusWidget::openProcessedSampleTab(int row, int /*col*/)
{
	//skip if not single sample
	if (jobs_[row].job_data.type!="single sample") return;

	QString ps = jobs_[row].job_data.samples[0].name;
	GlobalServiceProvider::openProcessedSampleTab(ps);
}

QTableWidgetItem* AnalysisStatusWidget::addItem(QTableWidget* table, int row, int col, QString text, QColor bg_color)
{
	QTableWidgetItem* item = new QTableWidgetItem(text);

	item->setFlags(Qt::ItemIsSelectable|Qt::ItemIsEnabled);
	item->setBackgroundColor(bg_color);
	table->setItem(row, col, item);

	return item;
}

QColor AnalysisStatusWidget::statusToColor(QString status)
{
	QColor output = Qt::transparent;
	if (status=="started") output = QColor("#90EE90");
	if (status=="finished") output = QColor("#44BB44");
	if (status=="cancel") output = QColor("#FFC45E");
	if (status=="canceled") output = QColor("#FFC45E");
	if (status=="error") output = QColor("#FF0000");
	return output;
}

QString AnalysisStatusWidget::timeHumanReadable(int sec)
{
	double m = sec / 60.0;
	double h = floor(m/60.0);
	m -= 60.0 * h;

	//create strings
	QString min = QString::number(m, 'f', 0) + "m";
	QString hours = h==0.0 ? "" : QString::number(h, 'f', 0) + "h ";

	return hours + min;
}

QList<int> AnalysisStatusWidget::selectedRows() const
{
	return GUIHelper::selectedTableRows(ui_.analyses);
}
