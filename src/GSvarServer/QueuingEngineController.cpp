#include "QueuingEngineController.h"
#include "QueuingEngineControllerSge.h"
#include "QueuingEngineControllerSlurm.h"
#include "Log.h"
#include "PipelineSettings.h"

QueuingEngineController* QueuingEngineController::create(const QString& engine)
{
	if (engine.toLower() == "sge") return new QueuingEngineControllerSge();
	else if (engine.toLower() == "slurm") return new QueuingEngineControllerSlurm();
	else
	{
		Log::error("Invalid queueing engine set in megSAP settings.ini: " + engine);
		return nullptr;
	}
}

void QueuingEngineController::run()
{
	QString engine = getEngineName();
	try
	{
		if (debug_) QTextStream(stdout) << engine << " update started" << Qt::endl;

		NGSD db;

		//process jobs
		SqlQuery query = db.getQuery();
		query.exec("SELECT id FROM analysis_job ORDER BY id ASC");
		while(query.next())
		{
			int job_id  = query.value("id").toInt();
			AnalysisJob job = db.analysisInfo(job_id);
			QString status = job.lastStatus();
			try
			{
				if(status=="queued")
				{
					job.checkValid(job_id);
					startAnalysis(db, job, job_id);
				}
				if(status=="started")
				{
					job.checkValid(job_id);
					updateAnalysisStatus(db, job, job_id);
				}
				if(status=="cancel")
				{
					job.checkValid(job_id);
					deleteJob(db, job, job_id);
				}
			}
			catch (Exception& e)
			{
				if (debug_) QTextStream(stdout) << engine << " job (id=" << QString::number(job_id) << ") update failed: " << e.message() << Qt::endl;
				Log::info(engine + " job (id=" + QString::number(job_id) + ") update failed: " + e.message());
			}
			catch (...)
			{
				if (debug_) QTextStream(stdout) << engine << " job (id=" << QString::number(job_id) << ") update failed with unkown error" << Qt::endl;
				Log::info(engine + " job (id=" + QString::number(job_id) + ") update failed with unkown error");
			}
		}

		//delete jobs that are older than 60 days
		query.exec("SELECT res.id FROM (SELECT j.id as id , MAX(jh.time) as last_update FROM analysis_job j, analysis_job_history jh WHERE jh.analysis_job_id=j.id GROUP BY j.id) as res WHERE res.last_update < SUBDATE(NOW(), INTERVAL 60 DAY)");
		while(query.next())
		{
			QString job_id = query.value("id").toString();
			if (debug_)
			{
				QTextStream(stdout) <<  "Removing job " << job_id << " because it is older than 60 days" << Qt::endl;
			}
			db.getQuery().exec("DELETE FROM `analysis_job_history` WHERE analysis_job_id=" + job_id);
			db.getQuery().exec("DELETE FROM `analysis_job_sample` WHERE analysis_job_id=" + job_id);
			db.getQuery().exec("DELETE FROM `analysis_job` WHERE id=" + job_id);
		}

		if (debug_) QTextStream(stdout) << engine << " update done" << Qt::endl;
	}
	catch (Exception& e)
	{
		if (debug_) QTextStream(stdout) << engine << " update failed: " << e.message() << Qt::endl;
		Log::info(engine + " status update failed: " + e.message());
	}
	catch (...)
	{
		if (debug_) QTextStream(stdout) << engine << " update failed with unkown error" << Qt::endl;
		Log::info(engine + " status update failed with unkown error");
	}
}

void QueuingEngineController::startAnalysis(NGSD& db, const AnalysisJob& job, int job_id)
{
	if (debug_) QTextStream(stdout) << "Starting job " << job_id << " (type: " << job.type << ")" << Qt::endl;

	//init
	QString timestamp = QDateTime::currentDateTime().toString("yyyyMMddhhmmss");
	QString engine = getEngineName();

	//get processed sample data
	QString project_folder;
	QList<ProcessedSampleData> ps_data;
	QList<QString> bams;
	QList<QString> infos;
	foreach(const AnalysisJobSample& s, job.samples)
	{
		QString ps_id = db.processedSampleId(s.name);

		ps_data << db.getProcessedSampleData(ps_id);
		bams << db.processedSamplePath(ps_id, PathType::BAM);
		infos << s.info;

		//get project folder from first sample
		if (project_folder.isEmpty())
		{
			project_folder = Helper::canonicalPath(QFileInfo(bams.last()).path()+"/../");
		}
	}

	//determine usable queues
	QStringList queues = PipelineSettings::queuesDefault();
	bool use_high_prio_queues = job.high_priority;
	foreach(const ProcessedSampleData& data, ps_data)
	{
		if (data.urgent) use_high_prio_queues = true;
	}
	if(use_high_prio_queues)
	{
		foreach(QString queue, PipelineSettings::queuesHighPriority())
		{
			if (!queues.contains(queue)) queues << queue;
		}
	}

	QString script;
	QStringList pipeline_args;
	if (job.type=="single sample")
	{
		QString folder = QFileInfo(bams[0]).path() + "/";
		pipeline_args << "-folder" << folder;
		pipeline_args << "-name" << ps_data[0].name;

		QString sys_type = ps_data[0].processing_system_type;
		if (sys_type=="RNA") //RNA
		{
			//only high_mem queues for RNA
			queues = PipelineSettings::queuesHighMemory();

			script = "analyze_rna.php";
			pipeline_args << "--log" << (folder+"analyze_rna_"+timestamp+".log");
		}
		else if (sys_type=="cfDNA (patient-specific)" || sys_type=="cfDNA")
		{
			script = "analyze_cfdna.php";
			pipeline_args << "--log" << (folder+"analyze_cfdna_"+timestamp+".log");
		}
		else if (sys_type=="lrGS") //longread WGS
		{
			script = "analyze_longread.php";

			pipeline_args << "--log" << (folder+"analyze_longread_"+timestamp+".log");
		}
		else //DNA
		{
			if (job.use_dragen)
			{
				script = "analyze_dragen.php";
				pipeline_args << "-user" << job.history.last().user;
				pipeline_args << "--log" << (folder+"analyze_dragen_"+timestamp+".log");
				if(job.high_priority) pipeline_args << "-high_priority";

				//switch to dragen queue
				queues = PipelineSettings::queuesDragen();
			}
			else
			{
				script = "analyze.php";
				pipeline_args << "--log" << (folder+"analyze_"+timestamp+".log");
			}

		}
	}
	else if (job.type=="trio")
	{
		//check if single analysis is still running > skip for now
		if(singleSampleAnalysisRunning(db, job))
		{
			if (debug_)
			{
				QTextStream(stdout) << "Job " << job_id << " (" << job.type << ") postponed because at least one single sample analysis is still running" << Qt::endl;
			}
			return;
		}

		//determine sample indices
		int c_idx = infos.indexOf("child");
		int f_idx = infos.indexOf("father");
		int m_idx = infos.indexOf("mother");

		//create output folder
		QString out_folder = project_folder + "/Trio_" + ps_data[c_idx].name + "_" + ps_data[f_idx].name + "_" + ps_data[m_idx].name + "/";
		if (Helper::mkdir(out_folder)==1)
		{
			if (!Helper::set777(out_folder))
			{
				db.addAnalysisHistoryEntry(job_id, "error", QByteArrayList() << ("Error while submitting analysis to " + engine.toLatin1() + ": Could not change privileges of folder " + out_folder.toLatin1()));
				return;
			}
		}

		script = (ps_data[0].processing_system_type=="lrGS") ? "trio_longread.php" : "trio.php";
		pipeline_args << "-c" << bams[c_idx];
		pipeline_args << "-f" << bams[f_idx];
		pipeline_args << "-m" << bams[m_idx];
		pipeline_args << "-out_folder" << out_folder;
		pipeline_args << "--log" << (out_folder + "/trio.log");
	}
	else if (job.type=="multi sample")
	{
		//check if single analysis is still running > skip for now
		if(singleSampleAnalysisRunning(db, job))
		{
			if (debug_)
			{
				QTextStream(stdout) << "Job " << job_id << " (" << job.type << ") postponed because at least one single sample analysis is still running" << Qt::endl;
			}
			return;
		}

		//create output folder
		QString out_folder = project_folder + "/Multi";
		foreach(const AnalysisJobSample& s, job.samples) out_folder += "_" + s.name;
		if (Helper::mkdir(out_folder)==1)
		{
			if (!Helper::set777(out_folder))
			{
				db.addAnalysisHistoryEntry(job_id, "error", QByteArrayList() << ("Error while submitting analysis to " + engine.toLatin1() + ": Could not change privileges of folder " + out_folder.toLatin1()));
				return;
			}
		}

		//determine command and arguments
		script = (ps_data[0].processing_system_type=="lrGS") ? "multisample_longread.php" : "multisample.php";
		pipeline_args << "-bams" << bams;
		pipeline_args << "-status" << infos;
		pipeline_args << "-out_folder" << out_folder;
		pipeline_args << "--log" << (out_folder + "/multi.log");
	}
	else if (job.type=="somatic")
	{
		//check if single analysis is still running > skip for now
		if(singleSampleAnalysisRunning(db, job))
		{
			if (debug_)
			{
				QTextStream(stdout) << "Job " << job_id << " (" << job.type << ") postponed because at least one single sample analysis is still running" << Qt::endl;
			}
			return;
		}

		//determine sample indices
		int t_idx = infos.indexOf("tumor");
		int n_idx = infos.indexOf("normal");
		int r_idx = infos.indexOf("tumor_rna");

		//create output folder
		QString out_folder;
		if (n_idx!=-1)
		{
			out_folder = project_folder + "/Somatic_" + ps_data[t_idx].name + "-" + ps_data[n_idx].name + "/";
		}
		else
		{
			out_folder = project_folder + "/Somatic_" + ps_data[t_idx].name + "/";
		}
		if (Helper::mkdir(out_folder)==1)
		{
			if (!Helper::set777(out_folder))
			{
				db.addAnalysisHistoryEntry(job_id, "error", QByteArrayList() << ("Error while submitting analysis to " + engine.toLatin1() + ": Could not change privileges of folder " + out_folder.toLatin1()));
				return;
			}
		}

		script = (n_idx!=-1) ? "somatic_tumor_normal.php" : "somatic_tumor_only.php";
		pipeline_args << "--log" << (out_folder+"/somatic_"+timestamp+".log");
		pipeline_args << "-out_folder" << out_folder;
		pipeline_args << "-prefix" << (ps_data[t_idx].name + (n_idx!=-1 ? "-"+ps_data[n_idx].name : ""));
		pipeline_args << "-t_bam" << bams[t_idx];
		if (n_idx!=-1)
		{
			pipeline_args << "-n_bam" << bams[n_idx];
		}
		if (r_idx!=-1)
		{
			pipeline_args << "-t_rna_bam" << bams[r_idx];
		}
		if (job.use_dragen) pipeline_args << "-use_dragen";

		if (ps_data[t_idx].processing_system_type == "WGS") queues = PipelineSettings::queuesHighMemory();
	}
	else
	{
		db.addAnalysisHistoryEntry(job_id, "error", QByteArrayList() << "Error while submitting analysis to " + engine.toLatin1() + ": Unknown analysis type '"+job.type.toLatin1()+"'!");
		return;
	}

	//determine number of threads to use (equal to the number of SGE/slurm slots)
	int threads = 4;
	foreach(const ProcessedSampleData& s, ps_data) //use more slots for WGS and RNA
	{
		if (s.processing_system_type=="WGS" || s.processing_system_type=="WGS (shallow)") threads = std::max(threads, 6);
		if (s.processing_system_type=="RNA") threads = std::max(threads, 5);
		if (s.processing_system_type=="lrGS") threads = std::max(threads, 8);
	}
	//handle number of threads when set in custom arguments
	bool threads_in_args = false;
	QStringList parts = job.args.simplified().split(' ');
	for (int i=0; i<parts.count()-1; ++i)
	{
		if (parts[i]=="-threads" && Helper::isNumeric(parts[i+1]))
		{
			threads_in_args = true;
			threads = Helper::toInt(parts[i+1]);
		}
	}
	if (!threads_in_args)
	{
		pipeline_args << "-threads" << QString::number(threads);
	}

	//job submission
	submitJob(db, threads, queues, pipeline_args, project_folder, script, job.args.simplified(), job_id);
}

void QueuingEngineController::updateAnalysisStatus(NGSD &db, const AnalysisJob &job, int job_id)
{
	if (debug_) QTextStream(stdout) << "Updating status of job " << job_id << " (type: " << job.type << " QE-id: " << job.sge_id << ")" << Qt::endl;


	//check if job is still running
	bool finished = updateRunningJob(db, job, job_id);

	if (finished) //finished => add status in NGSD
	{
		//load stdout/stderr output
		QByteArrayList stdout_stderr;
		QString engine = getEngineName().toLower();
		QString base = PipelineSettings::dataFolder() + "/" + engine + "/megSAP_" + engine + "_job_" + QString::number(job_id);
		if (QFile::exists(base+".out"))
		{
			foreach(QString line, Helper::loadTextFile(base+".out"))
			{
				stdout_stderr << line.toLatin1();
			}
		}
		if (QFile::exists(base+".err"))
		{
			foreach(QString line, Helper::loadTextFile(base+".err"))
			{
				stdout_stderr << line.toLatin1();
			}
		}

		checkCompletedJob(db, job.sge_id, stdout_stderr, job_id);
	}
}

bool QueuingEngineController::singleSampleAnalysisRunning(NGSD &db, const AnalysisJob &job)
{
	foreach (const AnalysisJobSample& sample, job.samples)
	{
		QString ps_id = db.processedSampleId(sample.name);

		SqlQuery query = db.getQuery();
		query.exec("SELECT j.id FROM analysis_job j, analysis_job_sample js WHERE j.type='single sample' AND js.analysis_job_id=j.id AND js.processed_sample_id='"+ps_id+"' ORDER BY id DESC");
		while(query.next())
		{
			AnalysisJob job_single = db.analysisInfo(query.value("id").toInt());
			if (job_single.isRunning()) return true;
		}

	}
	return false;
}

