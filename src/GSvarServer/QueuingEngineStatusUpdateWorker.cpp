#include "QueuingEngineStatusUpdateWorker.h"
#include "Log.h"
#include "PipelineSettings.h"
#include "QueuingEngineExecutorProviderSge.h"
#include "QueuingEngineExecutorProviderSlurm.h"

QueuingEngineStatusUpdateWorker::QueuingEngineStatusUpdateWorker()
    : QRunnable()
{
    if (PipelineSettings::queuingEngine() == "sge")
    {
        executor_provider_ = QSharedPointer<QueuingEngineExecutorProviderSge>(new QueuingEngineExecutorProviderSge());
    }
    if (PipelineSettings::queuingEngine() == "slurm")
    {
        executor_provider_ = QSharedPointer<QueuingEngineExecutorProviderSlurm>(new QueuingEngineExecutorProviderSlurm());
    }
}

void QueuingEngineStatusUpdateWorker::run()
{
	try
	{
        if (debug_) QTextStream(stdout) << "SGE update started" << QT_ENDL;

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
					canceledAnalysis(db, job, job_id);
				}
			}
			catch (Exception& e)
			{
                if (debug_) QTextStream(stdout) << "SGE job (id=" << QString::number(job_id) << ") update failed: " << e.message() << QT_ENDL;
				Log::info("SGE job (id=" + QString::number(job_id) + ") update failed: " + e.message());
			}
			catch (...)
			{
                if (debug_) QTextStream(stdout) << "SGE job (id=" << QString::number(job_id) << ") update failed with unkown error" << QT_ENDL;
				Log::info("SGE job (id=" + QString::number(job_id) + ") update failed with unkown error");
			}
		}

		//delete jobs that are older than 60 days
		query.exec("SELECT res.id FROM (SELECT j.id as id , MAX(jh.time) as last_update FROM analysis_job j, analysis_job_history jh WHERE jh.analysis_job_id=j.id GROUP BY j.id) as res WHERE res.last_update < SUBDATE(NOW(), INTERVAL 60 DAY)");
		while(query.next())
		{
			QString job_id = query.value("id").toString();
            if (debug_)
            {
                QTextStream(stdout) <<  "Removing job " << job_id << " because it is older than 60 days" << QT_ENDL;
            }
			db.getQuery().exec("DELETE FROM `analysis_job_history` WHERE analysis_job_id=" + job_id);
			db.getQuery().exec("DELETE FROM `analysis_job_sample` WHERE analysis_job_id=" + job_id);
			db.getQuery().exec("DELETE FROM `analysis_job` WHERE id=" + job_id);
		}

        if (debug_) QTextStream(stdout) << "SGE update done" << QT_ENDL;
	}
	catch (Exception& e)
	{
        if (debug_) QTextStream(stdout) << "SGE update failed: " << e.message() << QT_ENDL;
		Log::info("SGE status update failed: " + e.message());
	}
	catch (...)
	{
        if (debug_) QTextStream(stdout) << "SGE update failed with unkown error" << QT_ENDL;
		Log::info("SGE status update failed with unkown error");
	}

}

void QueuingEngineStatusUpdateWorker::startAnalysis(NGSD& db, const AnalysisJob& job, int job_id)
{
    if (debug_) QTextStream(stdout) << "Starting job " << job_id << " (type: " << job.type << ")" << QT_ENDL;

	//init
	QString timestamp = QDateTime::currentDateTime().toString("yyyyMMddhhmmss");

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
	if(job.high_priority)
	{
		queues = PipelineSettings::queuesHighPriority();
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
                QTextStream(stdout) << "Job " << job_id << " (" << job.type << ") postponed because at least one single sample analysis is still running" << QT_ENDL;
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
				db.addAnalysisHistoryEntry(job_id, "error", QByteArrayList() << ("Error while submitting analysis to SGE: Could not change privileges of folder " + out_folder.toLatin1()));
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
                QTextStream(stdout) << "Job " << job_id << " (" << job.type << ") postponed because at least one single sample analysis is still running" << QT_ENDL;
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
				db.addAnalysisHistoryEntry(job_id, "error", QByteArrayList() << ("Error while submitting analysis to SGE: Could not change privileges of folder " + out_folder.toLatin1()));
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
                QTextStream(stdout) << "Job " << job_id << " (" << job.type << ") postponed because at least one single sample analysis is still running" << QT_ENDL;
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
				db.addAnalysisHistoryEntry(job_id, "error", QByteArrayList() << ("Error while submitting analysis to SGE: Could not change privileges of folder " + out_folder.toLatin1()));
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
		db.addAnalysisHistoryEntry(job_id, "error", QByteArrayList() << "Error while submitting analysis to SGE: Unknown analysis type '"+job.type.toLatin1()+"'!");
		return;
	}

	//determine number of threads to use (equal to the number of SGE slots)
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
	QueuingEngineOutput qe_output = executor_provider_->submitJob(threads, queues, pipeline_args, project_folder, script, job.args.simplified(), QString::number(job_id), debug_);
	QByteArray engine_name = executor_provider_->getEngineName().toLatin1();
    if (qe_output.exit_code!=0)
	{
		QByteArrayList details;
		details << (engine_name + " job submission failed: returned exit code "+QByteArray::number(qe_output.exit_code)+"!");
		details << "Command:";
        details << (qe_output.command + " " + qe_output.args.join(" ")).toLatin1();
		details << "Output:";
        details << qe_output.result.join('\n');
		db.addAnalysisHistoryEntry(job_id, "error", details);
		return;
	}

	//determine queuing engine job number
	QByteArray sge_id;
	QByteArrayList qe_result = qe_output.result.join(" ").simplified().split(' ');
	if (engine_name == "SGE") sge_id = qe_result[2]; //sge result has format: "Your job 17 ("php") has been submitted"
	else if (engine_name == "SLURM") sge_id = qe_result[3]; //slurm result has format: "Submitted batch job 17"

	//handle qsub output
	if (Helper::isNumeric(sge_id) && sge_id.toInt()>0)
	{
        if (debug_) QTextStream(stdout) << "  Started with SGE id " << sge_id << QT_ENDL;
		db.getQuery().exec("UPDATE analysis_job SET sge_id='"+sge_id+"' WHERE id="+QString::number(job_id));

		db.addAnalysisHistoryEntry(job_id, "started", QByteArrayList());
	}
	else
	{
		QByteArrayList details;
		details << engine_name + " job submission failed - could not determine queuing engine job number!";
		details << "Command:";
		details << (qe_output.command + qe_output.args.join(" ")).toLatin1();
		details << "Output:";
        details << qe_output.result.join('\n');
		db.addAnalysisHistoryEntry(job_id, "error", details);
	}
}

void QueuingEngineStatusUpdateWorker::updateAnalysisStatus(NGSD& db, const AnalysisJob& job, int job_id)
{
    if (debug_) QTextStream(stdout) << "Updating status of job " << job_id << " (type: " << job.type << " SGE-id: " << job.sge_id << ")" << QT_ENDL;


	//check if job is still running
	QByteArray engine_name = executor_provider_->getEngineName().toLatin1();
    QueuingEngineOutput general_stats = executor_provider_->checkJobDetails(job.sge_id);
	if (general_stats.exit_code==0 && general_stats.result.size() > 1) //still running/queued > update NGSD infos if necessary
	{
        QueuingEngineOutput output = executor_provider_->checkJobsForAllUsers();
        if (output.exit_code==0)
		{
            foreach(QByteArray line, output.result)
			{
				line = line.simplified();
				if (!line.startsWith(job.sge_id.toLatin1() + ' ')) continue;
				QByteArrayList parts = line.split(' ');
				if (parts.count()<8) continue;

				QByteArray status = parts[4].trimmed().toLower();
                if (debug_) QTextStream(stdout) << "  Job queued/running (state: " << status << " queue: " << job.sge_queue << ")" << QT_ENDL;

				if (status=="r" && job.sge_queue.isEmpty())
				{
					QByteArray queue;
					if (engine_name == "SGE") queue = parts[7].split('@')[0].trimmed();
					else if (engine_name == "SLURM") queue = parts[1].split('@')[0].trimmed();

					SqlQuery query = db.getQuery();
					query.prepare("UPDATE analysis_job SET sge_queue=:0 WHERE id=:1");
					query.bindValue(0, queue);
					query.bindValue(1, job_id);
					query.exec();
				}
			}
		}
		else
		{
            Log::warn(output.command + " " + output.args.join(" ") + " failed - skipping update of SGE job with id " + job.sge_id);
		}
	}
	else //finished => add status in NGSD
	{
		//load stdout/stderr output
		QByteArrayList stdout_stderr;
		QString base = PipelineSettings::dataFolder() + "/sge/megSAP_sge_job_" + QString::number(job_id);
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

        QueuingEngineOutput output = executor_provider_->checkCompletedJob(job.sge_id);
        if (output.exit_code==0)
		{
			QString sge_exit_code = "";

			if (engine_name == "SGE")
			{
				foreach(QByteArray line, output.result)
				{
					line = line.simplified();
					if (line.startsWith("exit_status "))
					{
						sge_exit_code = line.split(' ')[1];
					}
				}
			}
			else if (engine_name == "SLURM")
			{
				foreach(QByteArray line, output.result)
				{
					if (!line.startsWith(job.sge_id.toLatin1() + ' ')) continue;
					QByteArrayList parts = line.simplified().split(' ');
					if (parts.size() >= 7)
					{
						QByteArray exit_code = parts.last();
						sge_exit_code = exit_code.split(':')[0];
					}
				}
			}

			if (sge_exit_code=="0")
			{
                if (debug_) QTextStream(stdout) << "	Job finished successfully" << QT_ENDL;
				db.addAnalysisHistoryEntry(job_id, "finished", stdout_stderr);
			}
			else
			{
                if (debug_) QTextStream(stdout) << "	Job failed with exit code: " << sge_exit_code << QT_ENDL;
				stdout_stderr.prepend(("job exit code: " + sge_exit_code).toLatin1());
				db.addAnalysisHistoryEntry(job_id, "error", stdout_stderr);
			}
		}
		else
		{
            Log::warn(output.command + " " + output.args.join(" ") + "' failed with exit code " + QString::number(output.exit_code));
		}
	}
}

void QueuingEngineStatusUpdateWorker::canceledAnalysis(NGSD& db, const AnalysisJob& job, int job_id)
{
    if (debug_) QTextStream(stdout) << "Canceling job " << job_id << " (type: " << job.type << " SGE-id: " << job.sge_id << ")" << QT_ENDL;

	//cancel job
    QueuingEngineOutput output;
	if (!job.sge_id.isEmpty()) // not started yet => nothing to cancel
	{
        output = executor_provider_->deleteJob(job.sge_id);
        if (output.exit_code!=0)
        {
            Log::warn(output.command + " " + job.sge_id + "' failed with exit code " + QString::number(output.exit_code));
		}
	}

	//update NGSD
    db.addAnalysisHistoryEntry(job_id, "canceled", output.result);
}

bool QueuingEngineStatusUpdateWorker::singleSampleAnalysisRunning(NGSD& db, const AnalysisJob& job)
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
